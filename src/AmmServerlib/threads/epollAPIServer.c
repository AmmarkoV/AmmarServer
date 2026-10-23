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
#include "apiProtocol.h"

#include "../server_configuration.h"
#include "../tools/logs.h"

/*
   Everything in this file runs on the single accept-epoll-layer thread ( EpollAcceptLayerThread ,
   threadedServer.c ) , so none of the state here is locked : there is exactly one of that thread , the API
   handler is only ever called from it , and one connection is only ever touched by it.

   Every function that can end up closing a connection reports that back through a connectionIsGone flag
   rather than a return value alone , because once CloseApiConnection has run the struct is freed and even
   reading a field off it is a use-after-free.
*/

struct ApiConnection
{
   enum EpollEventKind kind; //Always EPOLL_EVENT_KIND_API_CONNECTION ; must stay the first field , see threadedServer.h
   int clientsock;

   //Received bytes that did not add up to a complete request yet ( or that followed one , pipelined ). Empty
   //while the connection is idle , which is the normal state for nearly all of them nearly all of the time -
   //this is why an idle connection costs tens of bytes here instead of a receive buffer.
   struct ApiPartialRequest in;

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
//incomplete ( or pipelined-leftover ) request pays for a buffer of its own , in ApiConnection::in .
static char * apiScratchBuffer = 0;
//One response buffer is enough here : every response is sent , or copied into the connection's own queue ,
//before the next request is served on this thread.
static struct ApiResponseBuffer apiResponse = {0};

static int EnsureApiBuffers(struct AmmServer_Instance * instance)
{
  if (apiScratchBuffer==0)
   {
     apiScratchBuffer = (char*) malloc(API_MAX_REQUEST_SIZE+1);
     if (apiScratchBuffer==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }
   }

  if (apiResponse.memory==0)
   {
     if (!ApiProtocol_AllocateResponseBuffer(instance,&apiResponse)) { return 0; }
   }

  return 1;
}

static void CloseApiConnection(struct AmmServer_Instance * instance,struct ApiConnection * conn)
{
  epoll_ctl(instance->accept_epoll_fd,EPOLL_CTL_DEL,conn->clientsock,0);
  close(conn->clientsock);
  ApiProtocol_PartialFree(&conn->in);
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
  unsigned int length = ApiProtocol_StatusOnlyResponse(statusCode,response,sizeof(response));

  return SendApiResponse(instance,conn,response,length,1,connectionIsGone);
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
     struct ApiFrame frame;
     enum ApiFrameResult framing = ApiProtocol_NextFrame(buf,used,offset,&frame);

     if (framing==API_FRAME_INCOMPLETE) { break; } //header block or body still arriving

     if (framing==API_FRAME_REFUSED)
      {
        //Its framing cannot be worked out , so where the next request would start is unknown and nothing
        //behind it on this connection can be trusted either : answer and close , consuming nothing.
        SendApiStatusOnly(instance,conn,frame.refuseStatus,connectionIsGone);
        *consumed=offset;
        return 0;
      }

     //Worked out before serving , not after : ApiProtocol_Serve writes NUL terminators into the request line ,
     //so where the next request starts cannot be recovered from it once it has run.
     offset += frame.totalLength;

     char * response=0;
     unsigned int failStatus=0;
     unsigned int responseLength = ApiProtocol_Serve(instance,&frame,&apiResponse,&response,&failStatus);

     if (responseLength==0)
      {
        SendApiStatusOnly(instance,conn,failStatus,connectionIsGone);
        *consumed=offset;
        return 0;
      }

     if (!SendApiResponse(instance,conn,response,responseLength,!frame.keepAlive,connectionIsGone))
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

static void HandleApiReadable(struct AmmServer_Instance * instance,struct ApiConnection * conn)
{
  while (1)
   {
     //A connection with nothing buffered borrows the shared scratch buffer ; one that is mid-request reads into
     //its own, on top of what it already has.
     char * buf=0;
     unsigned int used=0;
     unsigned int space=0;

     if (conn->in.data!=0)
      {
        if (conn->in.used>=API_MAX_REQUEST_SIZE)
         {
           //A capped buffer with still no complete request in it is a request too big to ever be answered.
           int gone=0;
           SendApiStatusOnly(instance,conn,413,&gone);
           return;
         }

        //Reserving the space this read wants through the same append path the buffer grows by : appending
        //nothing cannot fail , so this only ever enlarges it.
        unsigned int want = conn->in.used + API_INITIAL_BUFFER_SIZE;
        if (want>API_MAX_REQUEST_SIZE) { want=API_MAX_REQUEST_SIZE; }
        if (!ApiProtocol_PartialReserve(&conn->in,want)) { CloseApiConnection(instance,conn); return; }

        buf   = conn->in.data;
        used  = conn->in.used;
        space = conn->in.size - used;
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
           if (!ApiProtocol_PartialKeepTail(&conn->in,buf,used,consumed)) { CloseApiConnection(instance,conn); }
         }
        return;
      }

     if (!ApiProtocol_PartialKeepTail(&conn->in,buf,used,consumed)) { CloseApiConnection(instance,conn); return; }

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
  if ( (conn->in.data!=0) && (conn->in.used>0) )
   {
     unsigned int consumed=0;
     int gone=0;
     if (!ServeBufferedApiRequests(instance,conn,conn->in.data,conn->in.used,&consumed,&gone))
      {
        if (!gone)
         {
           if (!ApiProtocol_PartialKeepTail(&conn->in,conn->in.data,conn->in.used,consumed)) { CloseApiConnection(instance,conn); }
         }
        return;
      }
     if (!ApiProtocol_PartialKeepTail(&conn->in,conn->in.data,conn->in.used,consumed)) { CloseApiConnection(instance,conn); }
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
