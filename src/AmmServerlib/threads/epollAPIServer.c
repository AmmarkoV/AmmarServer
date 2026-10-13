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
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>

#include "epollAPIServer.h"
#include "threadedServer.h"

#include "../server_configuration.h"
#include "../tools/http_tools.h"
#include "../tools/logs.h"

/*
   Everything in this file runs on the single accept-epoll-layer thread ( EpollAcceptLayerThread ,
   threadedServer.c ) , so none of the state here is locked : there is exactly one of that thread , the API
   handler is only ever called from it , and one connection is only ever touched by it.

   Every function that can end up closing a connection reports that back through a connectionIsGone flag
   rather than a return value alone , because once CloseApiConnection has run the struct is freed and even
   reading a field off it is a use-after-free.
*/

//One request has to fit in here , header block and body together. A REST API has no use for an unbounded
//request , and refusing anything bigger with a 413 is also what keeps a connection's worst case memory cost
//bounded when there are tens of thousands of them open.
#define API_MAX_REQUEST_SIZE (32*1024)
//What a connection that is left holding an incomplete request allocates to keep it, before growing. It
//matters at scale : handing every such connection a full API_MAX_REQUEST_SIZE buffer up front would mean
//65,000 connections could ask for 2 GB between them , where this grows to fit what is actually in flight.
#define API_INITIAL_BUFFER_SIZE 2048
//Room kept in front of the response body so the status line and headers can be written immediately before it ,
//making header+body one contiguous block that goes out in a single send() without the body being copied.
#define API_RESPONSE_HEADER_RESERVE 256
#define API_DEFAULT_MAX_RESPONSE_SIZE (64*1024)

struct ApiConnection
{
   enum EpollEventKind kind; //Always EPOLL_EVENT_KIND_API_CONNECTION ; must stay the first field , see threadedServer.h
   int clientsock;

   //Received bytes that did not add up to a complete request yet ( or that followed one , pipelined ). 0 while
   //the connection is idle , which is the normal state for nearly all of them nearly all of the time - this is
   //why an idle connection costs tens of bytes here instead of a receive buffer.
   char * in;
   unsigned int inSize;   //allocated , grows geometrically up to API_MAX_REQUEST_SIZE
   unsigned int inUsed;

   //Response bytes left over from a send() that could not take all of them. While this is set the connection
   //is registered for EPOLLOUT *instead* of EPOLLIN : reading the next request before this response is out
   //would reorder responses on a pipelined connection , and leaving EPOLLIN armed on a level-triggered epoll
   //while deliberately not reading would spin.
   char * out;
   unsigned int outTotal;
   unsigned int outSent;

   unsigned char closeWhenWritten;
};

//The one receive buffer every connection borrows for the usual case : a request that arrives complete in a
//single read and is fully consumed before the next connection is looked at. Only a connection left holding an
//incomplete ( or pipelined-leftover ) request pays for a buffer of its own , in ApiConnection::in.
static char * apiScratchBuffer = 0;
//Response body buffer handed to the API handler , with API_RESPONSE_HEADER_RESERVE bytes in front of it.
static char * apiResponseBuffer = 0;
static unsigned int apiResponseBufferBodySize = 0;

static int EnsureApiBuffers(struct AmmServer_Instance * instance)
{
  if (apiScratchBuffer==0)
   {
     apiScratchBuffer = (char*) malloc(API_MAX_REQUEST_SIZE+1);
     if (apiScratchBuffer==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }
   }

  if (apiResponseBuffer==0)
   {
     apiResponseBufferBodySize = instance->apiMaxResponseSize;
     if (apiResponseBufferBodySize==0) { apiResponseBufferBodySize = API_DEFAULT_MAX_RESPONSE_SIZE; }

     apiResponseBuffer = (char*) malloc(API_RESPONSE_HEADER_RESERVE+apiResponseBufferBodySize+1);
     if (apiResponseBuffer==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }
   }

  return 1;
}

static void CloseApiConnection(struct AmmServer_Instance * instance,struct ApiConnection * conn)
{
  epoll_ctl(instance->accept_epoll_fd,EPOLL_CTL_DEL,conn->clientsock,0);
  close(conn->clientsock);
  if (conn->in!=0)  { free(conn->in);  }
  if (conn->out!=0) { free(conn->out); }
  free(conn);
}

static int ArmApiConnection(struct AmmServer_Instance * instance,struct ApiConnection * conn,unsigned int eventBits)
{
  struct epoll_event ev={0};
  ev.events = eventBits;
  ev.data.ptr = conn;
  return ( epoll_ctl(instance->accept_epoll_fd,EPOLL_CTL_MOD,conn->clientsock,&ev)==0 );
}

int EpollAPI_AdoptConnection(struct AmmServer_Instance * instance,int clientsock)
{
  if (!EnsureApiBuffers(instance)) { return 0; }

  int flags = fcntl(clientsock,F_GETFL,0);
  if (flags<0) { return 0; }
  if (fcntl(clientsock,F_SETFL,flags|O_NONBLOCK)<0) { return 0; }

  //Responses are small and already one send() each , so Nagle would only add delay waiting for more.
  int yes=1;
  setsockopt(clientsock,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(yes));
  //A client that vanishes without a FIN would otherwise hold its fd and its struct forever , and with tens of
  //thousands of connections open that is the leak that eventually runs the process out of descriptors.
  setsockopt(clientsock,SOL_SOCKET,SO_KEEPALIVE,&yes,sizeof(yes));

  struct ApiConnection * conn = (struct ApiConnection *) calloc(1,sizeof(struct ApiConnection));
  if (conn==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }

  conn->kind = EPOLL_EVENT_KIND_API_CONNECTION;
  conn->clientsock = clientsock;

  struct epoll_event ev={0};
  ev.events = EPOLLIN|EPOLLRDHUP;
  ev.data.ptr = conn;
  if (epoll_ctl(instance->accept_epoll_fd,EPOLL_CTL_ADD,clientsock,&ev)!=0)
   {
     free(conn);
     return 0;
   }

  return 1;
}

const char * AmmServer_APIRequestHeader(const struct AmmServer_APIRequest * rqst,const char * headerName,unsigned int * valueLength)
{
  *valueLength=0;
  if ( (rqst==0) || (rqst->headers==0) || (headerName==0) ) { return 0; }

  unsigned int nameLength = strlen(headerName);
  const char * line = rqst->headers;
  const char * end  = rqst->headers + rqst->headersLength;

  while (line<end)
   {
     const char * lineEnd = (const char *) memchr(line,'\n',(size_t)(end-line));
     if (lineEnd==0) { lineEnd=end; }

     //The name has to start at the line start , which is also what keeps a request target like
     ///Content-Length:5 on the request line from ever being mistaken for a header.
     if ( ((unsigned int)(lineEnd-line) > nameLength) && (line[nameLength]==':') &&
          (strncasecmp(line,headerName,nameLength)==0) )
      {
        const char * value = line+nameLength+1;
        while ( (value<lineEnd) && ((*value==' ')||(*value=='\t')) ) { ++value; }

        const char * valueEnd = lineEnd;
        while ( (valueEnd>value) && ((valueEnd[-1]=='\r')||(valueEnd[-1]==' ')||(valueEnd[-1]=='\t')) ) { --valueEnd; }

        *valueLength = (unsigned int)(valueEnd-value);
        return value;
      }

     if (lineEnd==end) { break; }
     line = lineEnd+1;
   }

  return 0;
}

//Length of the method token is always handed back , even for a method this does not recognize by name , so the
//request target is found in the same place either way and the handler is the one that decides what an
//unsupported method deserves.
static int MethodFromRequestLine(const char * line,unsigned int length,unsigned int * methodLength)
{
  const char * space = (const char *) memchr(line,' ',length);
  *methodLength = (space!=0) ? (unsigned int)(space-line) : 0;

  switch (*methodLength)
   {
     case 3 : if (memcmp(line,"GET",3)==0)    { return GET;    }
              if (memcmp(line,"PUT",3)==0)    { return PUT;    }
     break;
     case 4 : if (memcmp(line,"POST",4)==0)   { return POST;   }
              if (memcmp(line,"HEAD",4)==0)   { return HEAD;   }
     break;
     case 6 : if (memcmp(line,"DELETE",6)==0) { return DELETE; }
     break;
   };

  return NONE;
}

//Where the header block ends , and how long its terminator was. Tolerates a bare LF between lines the same way
//find_header_terminator ( epollFastPathServer.c ) does , since real clients occasionally send it.
static int FindRequestHeaderEnd(const char * buf,unsigned int length,unsigned int * headerEnd)
{
  unsigned int i=0;
  for (i=0; i+1<length; i++)
   {
     if ( (buf[i]=='\n') && (buf[i+1]=='\n') )  { *headerEnd=i+2; return 1; }
     if ( (i+3<length) && (buf[i]=='\r') && (buf[i+1]=='\n') && (buf[i+2]=='\r') && (buf[i+3]=='\n') ) { *headerEnd=i+4; return 1; }
   }
  return 0;
}

//Queues whatever send() would not take and switches the connection over to waiting for EPOLLOUT.
static int DeferRemainingResponse(struct AmmServer_Instance * instance,struct ApiConnection * conn,const char * data,unsigned int length,int closeWhenWritten)
{
  conn->out = (char*) malloc(length);
  if (conn->out==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }

  memcpy(conn->out,data,length);
  conn->outTotal = length;
  conn->outSent  = 0;
  conn->closeWhenWritten = (closeWhenWritten!=0);

  return ArmApiConnection(instance,conn,EPOLLOUT|EPOLLRDHUP);
}

/**
* @brief Send one complete response , deferring to EPOLLOUT whatever the socket would not take.
* @retval 1=all of it went out and the connection is still ours , 0=stop serving it now ( check
          connectionIsGone to find out whether it still exists at all ) */
static int SendApiResponse(struct AmmServer_Instance * instance,struct ApiConnection * conn,const char * data,unsigned int length,int closeWhenWritten,int * connectionIsGone)
{
  *connectionIsGone=0;

  unsigned int sent=0;
  while (sent<length)
   {
     ssize_t res = send(conn->clientsock,data+sent,length-sent,MSG_NOSIGNAL|MSG_DONTWAIT);
     if (res>0) { sent+=(unsigned int) res; continue; }

     if ( (res<0) && (errno==EINTR) ) { continue; }

     if ( (res<0) && ((errno==EAGAIN)||(errno==EWOULDBLOCK)) )
      {
        if (!DeferRemainingResponse(instance,conn,data+sent,length-sent,closeWhenWritten))
         {
           CloseApiConnection(instance,conn);
           *connectionIsGone=1;
         }
        return 0;
      }

     CloseApiConnection(instance,conn); //client went away mid-response
     *connectionIsGone=1;
     return 0;
   }

  if (closeWhenWritten)
   {
     CloseApiConnection(instance,conn);
     *connectionIsGone=1;
     return 0;
   }

  return 1;
}

//Status-line-only response , for the few failures this layer answers by itself before any handler runs. All of
//them are fatal to the connection's framing , so all of them close it.
static int SendApiStatusOnly(struct AmmServer_Instance * instance,struct ApiConnection * conn,unsigned int statusCode,int * connectionIsGone)
{
  char response[API_RESPONSE_HEADER_RESERVE];
  int length = snprintf(response,sizeof(response),
                        "HTTP/1.1 %u %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
                        statusCode,GetHTTPReasonPhrase(statusCode));

  return SendApiResponse(instance,conn,response,(unsigned int) length,1,connectionIsGone);
}

/**
* @brief Run one fully received request through the instance's API handler and send what it produced.
* @retval 1=keep serving this connection , 0=stop touching it ( see connectionIsGone ) */
static int ServeOneApiRequest(
                               struct AmmServer_Instance * instance,
                               struct ApiConnection * conn,
                               char * request,
                               unsigned int headerEnd,
                               unsigned int bodyLength,
                               int keepAlive,
                               int * connectionIsGone
                             )
{
  *connectionIsGone=0;

  char * lineEnd = (char *) memchr(request,'\n',headerEnd);
  if (lineEnd==0) { return SendApiStatusOnly(instance,conn,400,connectionIsGone); }

  unsigned int methodLength=0;
  int method = MethodFromRequestLine(request,(unsigned int)(lineEnd-request),&methodLength);

  //Request line : METHOD SP target SP HTTP/x.y
  char * target = request + ( (methodLength>0) ? methodLength+1 : 0 );
  if (target>lineEnd) { return SendApiStatusOnly(instance,conn,400,connectionIsGone); }

  char * targetEnd = (char *) memchr(target,' ',(size_t)(lineEnd-target));
  if (targetEnd==0)
   {
     targetEnd=lineEnd; //No version token ( HTTP/0.9 style )
     while ( (targetEnd>target) && ((targetEnd[-1]=='\r')||(targetEnd[-1]==' ')) ) { --targetEnd; }
   }

  struct AmmServer_APIRequest rqst={0};
  rqst.method = method;

  //NUL terminating the path ( and query ) in place , over the request line's own delimiters. Nothing else in
  //the buffer can be terminated this cheaply : a NUL after the header block would land on the first body byte ,
  //so the handler gets headers as a pointer plus a length instead.
  char * queryStart = (char *) memchr(target,'?',(size_t)(targetEnd-target));
  *targetEnd=0;
  if (queryStart!=0)
   {
     *queryStart=0;
     rqst.query = queryStart+1;
     rqst.queryLength = (unsigned int)(targetEnd-(queryStart+1));
   }
  rqst.path = target;
  rqst.pathLength = strlen(target);

  char * headersStart = lineEnd+1;
  unsigned int headersLength = 0;
  if (headerEnd>(unsigned int)(headersStart-request))
   {
     headersLength = headerEnd - (unsigned int)(headersStart-request);
     while ( (headersLength>0) && ((headersStart[headersLength-1]=='\n')||(headersStart[headersLength-1]=='\r')) ) { --headersLength; }
   }
  rqst.headers = headersStart;
  rqst.headersLength = headersLength;

  rqst.body = (bodyLength>0) ? request+headerEnd : 0;
  rqst.bodyLength = bodyLength;

  struct AmmServer_APIResponse response={0};
  response.statusCode = 200;
  response.contentType = "application/json";
  response.body = apiResponseBuffer + API_RESPONSE_HEADER_RESERVE;
  response.bodyLength = 0;
  response.maxBodyLength = apiResponseBufferBodySize;

  void ( *DoCallback) (struct AmmServer_APIRequest *,struct AmmServer_APIResponse *) = 0;
  DoCallback = instance->apiHandler;
  DoCallback(&rqst,&response);

  if (response.bodyLength>apiResponseBufferBodySize)
   {
     //The handler has already overrun the buffer it was given , so the damage is done ; all this can do is
     //refuse to compound it by sending whatever is in the memory past it as if it were a response.
     errorID(ASV_ERROR_CLIENT_CAUSED_AN_OVERFLOW);
     fprintf(stderr,"API handler wrote %u bytes into a %u byte response buffer , serving %s \n",response.bodyLength,apiResponseBufferBodySize,rqst.path);
     response.bodyLength = apiResponseBufferBodySize;
   }

  char header[API_RESPONSE_HEADER_RESERVE];
  int headerLength = snprintf(header,sizeof(header),
                              "HTTP/1.1 %u %s\r\nContent-Type: %s\r\nContent-Length: %u\r\n%s\r\n",
                              response.statusCode,
                              GetHTTPReasonPhrase(response.statusCode),
                              (response.contentType!=0) ? response.contentType : "text/plain",
                              response.bodyLength,
                              keepAlive ? "" : "Connection: close\r\n" );

  if ( (headerLength<=0) || (headerLength>API_RESPONSE_HEADER_RESERVE) )
   {
     //Only reachable if a handler supplied an absurdly long Content-Type , which would truncate the header.
     return SendApiStatusOnly(instance,conn,500,connectionIsGone);
   }

  //Place the header immediately in front of the body the handler just wrote , so the whole response is one
  //contiguous block and leaves in a single send() with the body never being copied.
  char * responseStart = apiResponseBuffer + API_RESPONSE_HEADER_RESERVE - headerLength;
  memcpy(responseStart,header,(size_t) headerLength);

  //A HEAD response carries the Content-Length the GET would have , but none of the body.
  unsigned int responseLength = (unsigned int) headerLength + ( (method==HEAD) ? 0 : response.bodyLength );

  return SendApiResponse(instance,conn,responseStart,responseLength,!keepAlive,connectionIsGone);
}

/**
* @brief Answer every complete request sitting in buf , and report how much of it was consumed so the caller
*        can keep the rest for the next read.
* @retval 1=connection still alive and still ours , 0=stop touching it ( see connectionIsGone ) */
static int ServeBufferedApiRequests(
                                     struct AmmServer_Instance * instance,
                                     struct ApiConnection * conn,
                                     char * buf,
                                     unsigned int used,
                                     unsigned int * consumed,
                                     int * connectionIsGone
                                   )
{
  *connectionIsGone=0;
  unsigned int offset=0;

  while (offset<used)
   {
     char * request = buf+offset;
     unsigned int available = used-offset;

     unsigned int headerEnd=0;
     if (!FindRequestHeaderEnd(request,available,&headerEnd)) { break; } //header block still incomplete

     struct AmmServer_APIRequest probe={0};
     probe.headers = request;
     probe.headersLength = headerEnd;
     unsigned int valueLength=0;

     //Content-Length is what says where this request ends and the next one begins. Guessing wrong about that
     //would desynchronize every request behind it on this connection , so a framing this cannot work out
     //exactly is refused instead ( and a REST API has no use for a chunked request body ).
     const char * transferEncoding = AmmServer_APIRequestHeader(&probe,"Transfer-Encoding",&valueLength);
     if ( (transferEncoding!=0) && (valueLength>0) && (strncasecmp(transferEncoding,"identity",8)!=0) )
      {
        SendApiStatusOnly(instance,conn,501,connectionIsGone);
        *consumed=offset;
        return 0;
      }

     unsigned long bodyLength=0;
     const char * contentLength = AmmServer_APIRequestHeader(&probe,"Content-Length",&valueLength);
     if ( (contentLength!=0) && (valueLength>0) )
      {
        unsigned int i=0;
        for (i=0; i<valueLength; i++)
         {
           if ( (contentLength[i]<'0') || (contentLength[i]>'9') )
            {
              SendApiStatusOnly(instance,conn,400,connectionIsGone);
              *consumed=offset;
              return 0;
            }
           bodyLength = (bodyLength*10) + (unsigned long)(contentLength[i]-'0');
           if (bodyLength>API_MAX_REQUEST_SIZE)
            {
              SendApiStatusOnly(instance,conn,413,connectionIsGone);
              *consumed=offset;
              return 0;
            }
         }
      }

     if (available < headerEnd+bodyLength) { break; } //body still arriving

     //Keep-alive is HTTP/1.1's default ; HTTP/1.0 only gets it if it asks.
     int keepAlive = 1;
     if (memmem(request,headerEnd,"HTTP/1.0",8)!=0) { keepAlive=0; }

     const char * connectionHeader = AmmServer_APIRequestHeader(&probe,"Connection",&valueLength);
     if ( (connectionHeader!=0) && (valueLength>0) )
      {
        if (strncasecmp(connectionHeader,"close",5)==0)       { keepAlive=0; } else
        if (strncasecmp(connectionHeader,"keep-alive",10)==0) { keepAlive=1; }
      }

     //Worked out before serving , not after : ServeOneApiRequest writes NUL terminators into the request line ,
     //so where the next request starts cannot be recovered from it once it has run.
     offset += headerEnd + (unsigned int) bodyLength;

     if (!ServeOneApiRequest(instance,conn,request,headerEnd,(unsigned int) bodyLength,keepAlive,connectionIsGone))
      {
        //Either the connection is gone , or its response was deferred to EPOLLOUT - in which case anything
        //received behind it has to wait for that write to finish , or responses would come back out of order.
        *consumed=offset;
        return 0;
      }
   }

  *consumed=offset;
  return 1;
}

/**
* @brief Make sure this connection's own buffer can hold at least `needed` bytes , doubling it rather than
*        reallocating by the byte.
* @retval 1=it can , 0=it cannot ( needed is past the request cap , or out of memory ) */
static int EnsureInCapacity(struct ApiConnection * conn,unsigned int needed)
{
  if (needed>API_MAX_REQUEST_SIZE) { return 0; }
  if (conn->inSize>=needed)        { return 1; }

  unsigned int size = (conn->inSize>0) ? conn->inSize : API_INITIAL_BUFFER_SIZE;
  while (size<needed) { size*=2; }
  if (size>API_MAX_REQUEST_SIZE) { size=API_MAX_REQUEST_SIZE; }

  char * grown = (char*) realloc(conn->in,size+1);
  if (grown==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }

  conn->in = grown;
  conn->inSize = size;
  return 1;
}

/**
* @brief Take over from buf whatever was not consumed , so the next read continues it.
* @retval 1=ok , 0=could not , the connection has to be dropped */
static int KeepLeftoverForNextRead(struct ApiConnection * conn,char * buf,unsigned int used,unsigned int consumed)
{
  unsigned int leftover = used-consumed;

  if (leftover==0)
   {
     if (conn->in!=0) { free(conn->in); conn->in=0; conn->inSize=0; }
     conn->inUsed=0;
     return 1;
   }

  //When buf is already conn->in , leftover can never exceed what is allocated , so this cannot realloc and
  //cannot move the block out from under the memmove below. It only ever allocates for the scratch case.
  if (!EnsureInCapacity(conn,leftover)) { return 0; }

  memmove(conn->in,buf+consumed,leftover); //source and destination are the same buffer in the conn->in case
  conn->inUsed = leftover;
  return 1;
}

static void HandleApiReadable(struct AmmServer_Instance * instance,struct ApiConnection * conn)
{
  while (1)
   {
     //A connection with nothing buffered borrows the shared scratch buffer ; one that is mid-request reads into
     //its own, on top of what it already has.
     char * buf=0;
     unsigned int used=0;
     unsigned int space=0;

     if (conn->in!=0)
      {
        if (conn->inUsed>=API_MAX_REQUEST_SIZE)
         {
           //A capped buffer with still no complete request in it is a request too big to ever be answered.
           int gone=0;
           SendApiStatusOnly(instance,conn,413,&gone);
           return;
         }

        unsigned int want = conn->inUsed + API_INITIAL_BUFFER_SIZE;
        if (want>API_MAX_REQUEST_SIZE) { want=API_MAX_REQUEST_SIZE; }
        if (!EnsureInCapacity(conn,want)) { CloseApiConnection(instance,conn); return; }

        buf   = conn->in;
        used  = conn->inUsed;
        space = conn->inSize - used;
      } else
      {
        buf   = apiScratchBuffer;
        used  = 0;
        space = API_MAX_REQUEST_SIZE;
      }

     ssize_t res = recv(conn->clientsock,buf+used,space,MSG_DONTWAIT);

     if (res==0) { CloseApiConnection(instance,conn); return; } //orderly shutdown by the client

     if (res<0)
      {
        if (errno==EINTR) { continue; }
        if ( (errno==EAGAIN) || (errno==EWOULDBLOCK) ) { return; } //drained , epoll will say when there is more
        CloseApiConnection(instance,conn);
        return;
      }

     used += (unsigned int) res;

     unsigned int consumed=0;
     int gone=0;
     if (!ServeBufferedApiRequests(instance,conn,buf,used,&consumed,&gone))
      {
        //A deferred response now owns the connection , and the bytes behind it have to survive until it
        //completes ( HandleApiWritable picks them up ).
        if (!gone)
         {
           if (!KeepLeftoverForNextRead(conn,buf,used,consumed)) { CloseApiConnection(instance,conn); }
         }
        return;
      }

     if (!KeepLeftoverForNextRead(conn,buf,used,consumed)) { CloseApiConnection(instance,conn); return; }

     //A read that did not fill the space offered means the socket is drained , so another recv() would only
     //come back EAGAIN. Anything else probably has more waiting behind it.
     if ((unsigned int) res < space) { return; }
   }
}

static void HandleApiWritable(struct AmmServer_Instance * instance,struct ApiConnection * conn)
{
  while (conn->outSent<conn->outTotal)
   {
     ssize_t res = send(conn->clientsock,conn->out+conn->outSent,conn->outTotal-conn->outSent,MSG_NOSIGNAL|MSG_DONTWAIT);
     if (res>0) { conn->outSent+=(unsigned int) res; continue; }
     if ( (res<0) && (errno==EINTR) ) { continue; }
     if ( (res<0) && ((errno==EAGAIN)||(errno==EWOULDBLOCK)) ) { return; } //still backed up , wait for the next EPOLLOUT
     CloseApiConnection(instance,conn);
     return;
   }

  free(conn->out);
  conn->out=0;
  conn->outTotal=0;
  conn->outSent=0;

  if (conn->closeWhenWritten) { CloseApiConnection(instance,conn); return; }

  if (!ArmApiConnection(instance,conn,EPOLLIN|EPOLLRDHUP)) { CloseApiConnection(instance,conn); return; }

  //Requests that arrived while that response was draining are already in hand , and a level triggered epoll
  //will not report them again now that they have been read - they have to be picked up here.
  if ( (conn->in!=0) && (conn->inUsed>0) )
   {
     unsigned int consumed=0;
     int gone=0;
     if (!ServeBufferedApiRequests(instance,conn,conn->in,conn->inUsed,&consumed,&gone))
      {
        if (!gone)
         {
           if (!KeepLeftoverForNextRead(conn,conn->in,conn->inUsed,consumed)) { CloseApiConnection(instance,conn); }
         }
        return;
      }
     if (!KeepLeftoverForNextRead(conn,conn->in,conn->inUsed,consumed)) { CloseApiConnection(instance,conn); }
   }
}

void EpollAPI_HandleEvent(struct AmmServer_Instance * instance,void * connection,unsigned int events)
{
  struct ApiConnection * conn = (struct ApiConnection *) connection;

  if ( (events & EPOLLERR) || (events & EPOLLHUP) ) { CloseApiConnection(instance,conn); return; }

  //A half finished response decides what happens next , not the event bits. Dispatching on the bits instead
  //would hand an EPOLLRDHUP that arrives on its own ( peer closed its write side while our send buffer is
  //still full , so EPOLLOUT is not set ) to the read path , which would answer the next request ahead of the
  //response still queued in conn->out and overwrite that queue on the way. HandleApiWritable is also the
  //right answer for a peer that has gone away : the send either makes progress or fails and closes.
  if (conn->out!=0) { HandleApiWritable(instance,conn); return; }

  if ( (events & EPOLLIN) || (events & EPOLLRDHUP) ) { HandleApiReadable(instance,conn); return; }
}
