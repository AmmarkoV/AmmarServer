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

#ifndef APIPROTOCOL_H_INCLUDED
#define APIPROTOCOL_H_INCLUDED

#include "../AmmServerlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
   The HTTP/1.1 half of API mode : where a request ends , what it says , and the exact bytes of the answer.
   None of it touches a socket , which is what lets the same code serve a connection owned by epoll
   ( epollAPIServer.c ) and one owned by an io_uring ring ( uringAPIServer.c ). The transport owns the
   bytes in and the bytes out ; this owns everything in between.
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

//Where one response is laid out : the body the handler writes , with API_RESPONSE_HEADER_RESERVE bytes in
//front of it for the status line and headers , so header and body leave as one contiguous block. The
//transport owns these , because how many are needed at once is a transport question : epoll sends from one
//buffer synchronously and needs exactly one , while a ring has to keep every buffer it has submitted a send
//for alive until that send completes.
struct ApiResponseBuffer
{
   char * memory;
   unsigned int bodySize;
};

//The bytes a connection is left holding : a request that has not fully arrived , what followed one on a
//pipelined connection , or what turned up while its response was still going out. Zero for an idle
//connection , which is what makes one cheap enough to have tens of thousands of.
struct ApiPartialRequest
{
   char * data;
   unsigned int size;  //allocated , grows geometrically up to API_MAX_REQUEST_SIZE
   unsigned int used;
};

enum ApiFrameResult
{
  API_FRAME_INCOMPLETE = 0, //not all of this request has arrived yet , read more and ask again
  API_FRAME_READY,          //a complete request , ready to be served
  API_FRAME_REFUSED         //its framing cannot be worked out , answer refuseStatus and close
};

//One complete request found inside a receive buffer , and what the transport has to know about it.
struct ApiFrame
{
   char * request;            //where it starts , inside the caller's buffer
   unsigned int headerEnd;    //length of the header block , including its terminator
   unsigned int bodyLength;
   unsigned int totalLength;  //headerEnd+bodyLength : what the caller consumes for this request
   int keepAlive;
   unsigned int refuseStatus; //only set for API_FRAME_REFUSED
};

/**
* @brief Allocate one response buffer , sized by the instance's apiMaxResponseSize.
* @retval 1=allocated , 0=out of memory */
int ApiProtocol_AllocateResponseBuffer(struct AmmServer_Instance * instance,struct ApiResponseBuffer * buffer);

/**
* @brief Find the next complete request in buf starting at offset.
* @retval see enum ApiFrameResult */
enum ApiFrameResult ApiProtocol_NextFrame(char * buf,unsigned int used,unsigned int offset,struct ApiFrame * frame);

/**
* @brief The first half of ApiProtocol_Serve : turn one framed request into what a handler is given , with the
*        response's body pointing into buffer. The request points into the frame's own bytes , so those have to
*        stay where they are until ApiProtocol_Finish. Split out so that a transport can prepare several
*        requests , answer them with one batch handler call , and then finish each.
* @retval 1=ready for a handler , 0=it cannot be served ( answer failStatus and close ) */
int ApiProtocol_Prepare(
                         struct ApiFrame * frame,
                         struct ApiResponseBuffer * buffer,
                         struct AmmServer_APIRequest * rqst,
                         struct AmmServer_APIResponse * response,
                         unsigned int * failStatus
                       );

/**
* @brief The second half of ApiProtocol_Serve : once a handler has filled response , write its status line and
*        headers in front of the body , so the whole response is one contiguous block in buffer.
* @retval length of the response , or 0 if it could not be built ( answer failStatus and close ) */
unsigned int ApiProtocol_Finish(
                                 struct ApiFrame * frame,
                                 struct ApiResponseBuffer * buffer,
                                 const struct AmmServer_APIRequest * rqst,
                                 struct AmmServer_APIResponse * response,
                                 char ** responseBytes,
                                 unsigned int * failStatus
                               );

/**
* @brief Run one framed request through the instance's API handler and lay out the response bytes in buffer.
* @retval length of the response , or 0 if it could not be built ( answer failStatus and close ) */
unsigned int ApiProtocol_Serve(
                                struct AmmServer_Instance * instance,
                                struct ApiFrame * frame,
                                struct ApiResponseBuffer * buffer,
                                char ** response,
                                unsigned int * failStatus
                              );

/**
* @brief Keep buf[consumed..used) as this connection's partial request , for the next read to continue.
*        buf may be the partial buffer itself , which is how a consumed prefix is dropped.
* @retval 1=kept , 0=it cannot be ( past the request cap , or out of memory ) : drop the connection */
int ApiProtocol_PartialKeepTail(struct ApiPartialRequest * partial,const char * buf,unsigned int used,unsigned int consumed);

/**
* @brief Append length bytes to this connection's partial request.
* @retval 1=appended , 0=it cannot be ( past the request cap , or out of memory ) : drop the connection */
int ApiProtocol_PartialAppend(struct ApiPartialRequest * partial,const char * from,unsigned int length);

/**
* @brief Grow the buffer so it can hold `needed` bytes , without changing what is in it.
* @retval 1=it can , 0=it cannot ( past the request cap , or out of memory ) */
int ApiProtocol_PartialReserve(struct ApiPartialRequest * partial,unsigned int needed);

void ApiProtocol_PartialFree(struct ApiPartialRequest * partial);

/**
* @brief The status-line-only response the transport sends for a failure of its own , into its own buffer.
* @retval its length in bytes */
unsigned int ApiProtocol_StatusOnlyResponse(unsigned int statusCode,char * into,unsigned int intoSize);

#ifdef __cplusplus
}
#endif

#endif // APIPROTOCOL_H_INCLUDED
