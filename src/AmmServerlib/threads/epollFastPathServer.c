/*
AmmarServer , HTTP Server Library

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "epollFastPathServer.h"
#include "threadedServer.h"
#include "prespawnedThreads.h"
#include "freshThreads.h"

#include "../version.h"
#include "../server_configuration.h"
#include "../cache/file_caching.h"
#include "../cache/client_list.h"
#include "../tools/http_tools.h"
#include "../tools/time_provider.h"
#include "../tools/logs.h"

/*
   This whole file is an additive fast path : every eligibility check below either confirms a request is a
   simple, already-cached, static GET/HEAD ( no query string , no Range/ETag/compression negotiation , no
   password protection , not a dynamic resource ) BEFORE a single byte is consumed from the socket ( MSG_PEEK
   only ) , or it bails out and lets the caller dispatch to a worker thread exactly as if this file didn't
   exist. Path resolution/security reuses the same primitives ProcessFirstHTTPLine() uses ( StripHTMLCharacters_Inplace ,
   FilenameStripperOk , ReducePathSlashes_Inplace ) rather than reimplementing path-traversal validation here.

   IMPORTANT lesson learned benchmarking v1 of this file : the original design handed a connection back to the
   shared accept-epoll thread ( an epoll_ctl round trip + waiting for epoll_wait() to report it again ) between
   EVERY request , the same way it does for a genuinely idle new connection. That measured ~3x SLOWER than the
   plain thread-per-connection loop for a benchmark hammering one cached file over keep-alive connections ( 46K
   vs 138K req/s ) - waking a *different* thread through epoll costs more scheduling latency than a worker
   thread just blocking in its own recv() and being woken directly by the kernel, and funnelling many
   connections' continuations through one shared thread throws away the parallelism N worker threads had.
   EpollFastPath_TryServe() below therefore only avoids a thread entirely for a connection's first request (
   already a clean win - no thread would have been used at all otherwise ) plus any of its own truly back-to-back
   pipelined requests still sitting in the socket buffer right now ; the moment that runs dry it hands the
   connection to a normal worker thread ( DispatchContinuationToWorker() ) for whatever comes next, exactly like
   a fresh accept would have. It deliberately does NOT try to keep idle-but-still-open connections thread-free
   for their 2nd+ request - that would require a genuinely different design ( e.g. one event-loop-thread per
   core, each owning a subset of connections ) to pay off, which is out of scope for this pass.
*/

#define FASTPATH_PEEK_CAP 2048
//Keeps a fast-path response ( header + whole body copied into one buffer for a single send() ) bounded , so
//even a from-scratch non-blocking send of it is cheap. Bigger/uncached files fall back to TransmitFileToSocket.
#define FASTPATH_MAX_BODY_SIZE (256*1024)
//Upper bound on how many back-to-back requests one EpollFastPath_TryServe() call will serve off the same
//connection before yielding back to epoll_wait() , so one very chatty/pipelining connection can't monopolize
//this thread and starve every other connection's turn.
#define FASTPATH_MAX_PIPELINED_REQUESTS 64

enum FastPathOneRequestResult
{
   FASTPATH_INELIGIBLE = 0, //nothing was consumed from the socket
   FASTPATH_DONE        ,   //fully handled ( closed , or a write was deferred to EPOLLOUT ) - stop , don't recycle
   FASTPATH_AGAIN            //served one request , connection is keep-alive and still open - safe to try again now
};

struct FastPathPendingWrite
{
   enum EpollEventKind kind; //Always EPOLL_EVENT_KIND_FASTPATH_WRITE ; must stay the first field
   int clientsock;
   char * buffer; //owns it , freed once the write finishes or fails
   unsigned int total;
   unsigned int sent;
   int keepalive;
};

static int find_header_terminator(const char * buf,unsigned int len,unsigned int * header_len)
{
  unsigned int i;
  for (i=0; i+1<len; i++)
   {
     if ( (buf[i]=='\n') && (buf[i+1]=='\n') ) { *header_len=i+2; return 1; }
     if ( (i+3<len) && (buf[i]=='\r') && (buf[i+1]=='\n') && (buf[i+2]=='\r') && (buf[i+3]=='\n') ) { *header_len=i+4; return 1; }
   }
  return 0;
}

static enum FastPathOneRequestResult TryServeOneRequest(struct AmmServer_Instance * instance,int clientsock)
{
  char peek[FASTPATH_PEEK_CAP+1];
  ssize_t peeked = recv(clientsock,peek,FASTPATH_PEEK_CAP,MSG_PEEK|MSG_DONTWAIT);
  if (peeked<5) { return FASTPATH_INELIGIBLE; } //no data yet , closed , error , or too small to be GET/HEAD
  peek[peeked]=0;

  int is_head=0;
  if (strncmp(peek,"GET ",4)==0)       { is_head=0; }
  else if (strncmp(peek,"HEAD ",5)==0) { is_head=1; }
  else { return FASTPATH_INELIGIBLE; } //anything else ( POST , PUT , ... ) -> normal pipeline

  unsigned int header_len=0;
  if (!find_header_terminator(peek,(unsigned int) peeked,&header_len)) { return FASTPATH_INELIGIBLE; } //header not fully buffered yet

  //Conservative disqualifiers - a false positive here just costs an unnecessary fallback , never a wrong response.
  //Every one of these is already handled correctly by SendFile()'s existing pipeline , so we simply don't try to
  //duplicate that logic here.
  if ( strcasestr(peek,"\nRange:")!=0            ||
       strcasestr(peek,"\nIf-None-Match:")!=0    ||
       strcasestr(peek,"\nIf-Modified-Since:")!=0||
       strcasestr(peek,"\nAccept-Encoding:")!=0  ||
       strcasestr(peek,"\nAuthorization:")!=0 )
   { return FASTPATH_INELIGIBLE; }

  unsigned int method_len = is_head?5:4;
  char * path_start = peek+method_len;
  char * sp = (char *) memchr(path_start,' ',(size_t)(peeked-method_len));
  if (sp==0) { return FASTPATH_INELIGIBLE; }
  unsigned int path_len = (unsigned int)(sp-path_start);
  if ( (path_len==0) || (path_len>=MAX_RESOURCE-2) ) { return FASTPATH_INELIGIBLE; }
  if (memchr(path_start,'?',path_len)!=0) { return FASTPATH_INELIGIBLE; } //has a query string -> not a "simple static GET"

  char resource[MAX_RESOURCE+1]={0};
  memcpy(resource,path_start,path_len);
  resource[path_len]=0;

  //Reuse the exact same security-critical primitives ProcessFirstHTTPLine() uses - do not reimplement
  //path-traversal validation here.
  StripHTMLCharacters_Inplace(resource,1);
  if (!FilenameStripperOk(resource)) { return FASTPATH_INELIGIBLE; }

  char verified_filename[(MAX_FILE_PATH*2)+1]={0};
  strncpy(verified_filename,instance->webserver_root,MAX_FILE_PATH);
  strncat(verified_filename,resource,MAX_FILE_PATH);
  ReducePathSlashes_Inplace(verified_filename);

  unsigned int index=0;
  if (!cache_ResourceExists(instance,verified_filename,&index)) { return FASTPATH_INELIGIBLE; } //uncached/on-disk -> normal pipeline

  struct cache_item * cache = (struct cache_item *) instance->cache;
  if (cache[index].dynamicRequestCallbackFunction!=0) { return FASTPATH_INELIGIBLE; } //dynamic resource -> callback pipeline , unchanged
  if (cache[index].doNOTCacheRule) { return FASTPATH_INELIGIBLE; }
  if ( (cache[index].content==0) || (cache[index].contentSize==0) ||
       (*cache[index].contentSize==0) || (*cache[index].contentSize>FASTPATH_MAX_BODY_SIZE) )
   { return FASTPATH_INELIGIBLE; } //not a simple in-memory buffer this fast path is willing to handle - all still pre-consume

  //keepalive default : matches ProcessFirstHTTPLine()/AnalyzeHTTPLineRequest()'s HTTPHEADER_CONNECTION logic -
  //HTTP/1.1 defaults to keep-alive unless the client explicitly asks to close ; HTTP/1.0 defaults the other
  //way , keep-alive there is opt-in only. This is a byte-peeking approximation of that same logic ( no
  //structured header parsing here ) , close enough given a miscategorization here just costs a connection
  //being closed/kept a little differently than the real parser would , not a correctness issue.
  int is_http_1_0 = (strcasestr(peek," HTTP/1.0")!=0);
  int says_close  = (strcasestr(peek,"\nConnection: close")!=0) || (strcasestr(peek,"\nConnection:close")!=0);
  int says_keepalive = (strcasestr(peek,"keep-alive")!=0);
  int keepalive = is_http_1_0 ? says_keepalive : !says_close;

  //--- Eligible. Everything past this point actually consumes the request. ---
  char discard[FASTPATH_PEEK_CAP+1];
  ssize_t got = recv(clientsock,discard,header_len,MSG_DONTWAIT);
  if (got != (ssize_t) header_len)
   { //Data was right there moments ago via MSG_PEEK - this really shouldn't happen. Don't risk serving on top
     //of a partially consumed header, just drop the connection.
     close(clientsock);
     return FASTPATH_DONE;
   }

  ++instance->statistics.recvOperationsStarted;
  ++instance->statistics.recvOperationsFinished;
  instance->statistics.totalDownloadKB += (unsigned long) header_len/1024;

  struct HTTPHeader dummyRequest={0}; //cache_GetResource only reads compression-negotiation fields from this ,
                                       //which this fast path has already excluded ( no Accept-Encoding ) above
  unsigned long cached_lSize=0;
  unsigned char compressionSupported=0;
  unsigned char freeContentAfterUsingIt=0;
  unsigned char serveAsRegularFile=0;
  unsigned char allowOtherOrigins=0;
  char * cached_buffer = cache_GetResource(instance,&dummyRequest,index,verified_filename,sizeof(verified_filename),
                                            &index,&cached_lSize,0,&compressionSupported,&freeContentAfterUsingIt,
                                            &serveAsRegularFile,&allowOtherOrigins,0,0); //Fast path never serves dynamic
                                            //content ( see top-of-file design comment ) so it can never have a pending
                                            //Set-Cookie header to emit - no output buffer needed here.

  if ( (cached_buffer==0) || (cached_lSize==0) || (cached_lSize>FASTPATH_MAX_BODY_SIZE) || (compressionSupported) )
   { //Defensive only - the pre-consume check above should already guarantee this doesn't happen
     freeMallocIfNeeded(cached_buffer,freeContentAfterUsingIt);
     close(clientsock);
     return FASTPATH_DONE;
   }

  char content_type[MAX_CONTENT_TYPE+1]={0};
  strncpy(content_type,"text/html",MAX_CONTENT_TYPE);
  GetContentType(verified_filename,content_type,MAX_CONTENT_TYPE);

  //Truncated to unsigned int and formatted the exact same way SendFile()'s cached-buffer branch does ( with
  //start_at_byte=end_at_byte=0 , since Range requests are excluded from this fast path ) , so a resource's
  //ETag is byte-identical whichever path served it - a client may see one response via each across requests.
  unsigned int cache_etag = (unsigned int) cache_GetHashOfResource(instance,index);

  char headerbuf[MAX_HTTP_REQUEST_HEADER_REPLY+1]={0};
  snprintf(headerbuf,MAX_HTTP_REQUEST_HEADER_REPLY,
           "HTTP/1.1 200 OK\nServer: Ammarserver/%s\nContent-type: %s\nCache-Control: max-age=3600\nAccept-Ranges: bytes\n",
           FULLVERSION_STRING,content_type);
  //Using strlen() of the ( possibly truncated by snprintf ) buffer rather than snprintf's own return value keeps
  //this offset from ever exceeding the buffer , matching the same pattern SendSuccessCodeHeader() already uses.
  int off = (int) strlen(headerbuf);
  GetDateString(headerbuf+off,MAX_HTTP_REQUEST_HEADER_REPLY-off,"Date",1,0,0,0,0,0,0,0);
  off = (int) strlen(headerbuf);

  if (allowOtherOrigins)
   {
     off += snprintf(headerbuf+off,MAX_HTTP_REQUEST_HEADER_REPLY-off,"Access-Control-Allow-Origin: *\n");
   }
  if (cache_etag!=0)
   {
     off += snprintf(headerbuf+off,MAX_HTTP_REQUEST_HEADER_REPLY-off,"ETag: \"%u%u%lu%lu\"\n",instance->cacheVersionETag,cache_etag,(unsigned long)0,(unsigned long)0);
   }
  off += snprintf(headerbuf+off,MAX_HTTP_REQUEST_HEADER_REPLY-off,"Connection: %s\n",keepalive?"keep-alive":"close");
  off += snprintf(headerbuf+off,MAX_HTTP_REQUEST_HEADER_REPLY-off,"Content-length: %lu\r\n\r\n",cached_lSize);

  unsigned int header_out_len = (unsigned int) strlen(headerbuf);
  unsigned int body_len = is_head?0:(unsigned int) cached_lSize;
  unsigned int total = header_out_len+body_len;

  char * responseBuf = (char *) malloc(total);
  if (responseBuf==0)
   {
     freeMallocIfNeeded(cached_buffer,freeContentAfterUsingIt);
     close(clientsock);
     return FASTPATH_DONE;
   }
  memcpy(responseBuf,headerbuf,header_out_len);
  if (!is_head) { memcpy(responseBuf+header_out_len,cached_buffer,body_len); }
  freeMallocIfNeeded(cached_buffer,freeContentAfterUsingIt); //copied what we need , done with the cache's own buffer

  char ipStr[MAX_IP_STRING_SIZE]={0}; unsigned int port=0;
  getSocketIPAddress(instance,clientsock,ipStr,MAX_IP_STRING_SIZE,&port);
  clientID cid = clientList_GetClientId(instance->clientList,ipStr);

  ssize_t sent = send(clientsock,responseBuf,total,MSG_DONTWAIT|MSG_NOSIGNAL);
  if (sent==(ssize_t) total)
   {
     free(responseBuf);
     AccessLogAppend(ipStr,0,0,200,(unsigned long) body_len,verified_filename,0);
     clientList_signalClientStoppedUsingResource(instance->clientList,cid,resource);
     if (!keepalive) { close(clientsock); return FASTPATH_DONE; }
     return FASTPATH_AGAIN; //caller re-peeks immediately instead of going back through epoll
   }

  if ( (sent<0) && (errno!=EAGAIN) && (errno!=EWOULDBLOCK) )
   {
     free(responseBuf);
     close(clientsock);
     return FASTPATH_DONE;
   }

  //Partial write , or EAGAIN with nothing sent yet - queue the remainder for EPOLLOUT completion. The deferred
  //path intentionally skips access-logging/QoS-signalling above for simplicity ( a documented v1 limitation -
  //it only affects the rare case where the kernel send buffer is already full ).
  struct FastPathPendingWrite * pw = (struct FastPathPendingWrite *) malloc(sizeof(struct FastPathPendingWrite));
  if (pw==0) { free(responseBuf); close(clientsock); return FASTPATH_DONE; }
  pw->kind=EPOLL_EVENT_KIND_FASTPATH_WRITE;
  pw->clientsock=clientsock;
  pw->buffer=responseBuf;
  pw->total=total;
  pw->sent=(sent>0)?(unsigned int) sent:0;
  pw->keepalive=keepalive;

  struct epoll_event ev={0};
  ev.events=EPOLLOUT|EPOLLONESHOT;
  ev.data.ptr=pw;
  epoll_ctl(instance->accept_epoll_fd,EPOLL_CTL_DEL,clientsock,0); //no-op if it wasn't registered
  if (epoll_ctl(instance->accept_epoll_fd,EPOLL_CTL_ADD,clientsock,&ev)!=0)
   {
     free(pw->buffer); free(pw); close(clientsock); return FASTPATH_DONE;
   }
  return FASTPATH_DONE;
}

//Once the fast path has served at least one request on a connection and then runs out of immediately-available
//work ( genuinely idle for the moment , the next request isn't fast-path eligible , or the pipelining cap was
//hit ) , this hands the connection to a normal worker thread for whatever comes next - exactly the same
//prespawned/fresh dispatch a brand new accept goes through. Deliberately NOT handed back to the shared epoll
//thread here : benchmarking showed recycling an already-active connection back through it for its next request
//costs far more ( an extra thread wake-up hop through a different thread ) than just letting a worker thread
//block in recv() on it directly , the same way it always has. The one-time cost of spinning up that worker
//thread is a small, fixed price - not a per-request tax like the epoll round trip was.
static void DispatchContinuationToWorker(struct AmmServer_Instance * instance,int clientsock)
{
  struct sockaddr_in dummy_client; memset(&dummy_client,0,sizeof(dummy_client));
  unsigned int dummy_clientlen = sizeof(dummy_client);

  #if SINGLE_THREAD_MODE
  if (SingleThreadToServeNewClient(instance,clientsock,dummy_client,dummy_clientlen,0)) { return; }
  #else
  if (UsePreSpawnedThreadToServeNewClient(instance,clientsock,dummy_client,dummy_clientlen,instance->webserver_root,instance->templates_root,0)) { return; }
  if (SpawnThreadToServeNewClient(instance,clientsock,dummy_client,dummy_clientlen,0)) { return; }
  #endif // SINGLE_THREAD_MODE

  close(clientsock); //out of resources to serve this connection any further
}

int EpollFastPath_TryServe(struct AmmServer_Instance * instance,int clientsock)
{
  if (instance==0) { return 0; }
  if (instance->settings.PASSWORD_PROTECTION) { return 0; }
  if (instance->cache==0) { return 0; }

  int servedAny=0;
  unsigned int iterations=0;

  while (iterations<FASTPATH_MAX_PIPELINED_REQUESTS)
   {
     ++iterations;
     enum FastPathOneRequestResult result = TryServeOneRequest(instance,clientsock);

     if (result==FASTPATH_AGAIN) { servedAny=1; continue; } //more of this connection's own pipelined requests -
                                                              //keep going without any hand-off at all
     if (result==FASTPATH_DONE)  { return 1; }

     //FASTPATH_INELIGIBLE : nothing was consumed for this ( possibly first ) request.
     if (!servedAny) { return 0; } //cold miss - let the caller dispatch to a worker directly , zero extra cost

     DispatchContinuationToWorker(instance,clientsock);
     return 1;
   }

  //Hit the pipelining cap - this connection is clearly busy , so a worker thread is the right place for it to
  //keep going rather than yielding back to this shared thread again.
  DispatchContinuationToWorker(instance,clientsock);
  return 1;
}

void EpollFastPath_ResumeWrite(struct AmmServer_Instance * instance,void * pending_write)
{
  struct FastPathPendingWrite * pw = (struct FastPathPendingWrite *) pending_write;
  if (pw==0) { return; }

  while (pw->sent < pw->total)
   {
     ssize_t n = send(pw->clientsock,pw->buffer+pw->sent,pw->total-pw->sent,MSG_DONTWAIT|MSG_NOSIGNAL);
     if (n>0) { pw->sent += (unsigned int) n; continue; }

     if ( (n<0) && ( (errno==EAGAIN) || (errno==EWOULDBLOCK) ) )
      {
        struct epoll_event ev={0};
        ev.events=EPOLLOUT|EPOLLONESHOT;
        ev.data.ptr=pw;
        if (instance->accept_epoll_fd>=0) { epoll_ctl(instance->accept_epoll_fd,EPOLL_CTL_MOD,pw->clientsock,&ev); }
        return; //still owned by the epoll set , not freed - we'll be resumed again
      }

     //Hard error or the peer closed mid-response
     free(pw->buffer);
     int fd=pw->clientsock;
     free(pw);
     close(fd);
     return;
   }

  int fd=pw->clientsock;
  int keepalive=pw->keepalive;
  free(pw->buffer);
  free(pw);
  if (keepalive) { DispatchContinuationToWorker(instance,fd); } else { close(fd); }
}
