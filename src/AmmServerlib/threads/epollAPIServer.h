/** @file epollAPIServer.h
* @brief API mode ( AmmServer_SetAPIHandler ) : one non-blocking epoll event loop that owns every connection
*        for its entire life and answers each request from a single C callback that returns its own status
*        code , content type and body.
*
*        The thread-per-connection pipeline this replaces is the faster choice when a connection is busy -
*        a worker blocking in its own recv() is woken by the kernel directly , with no epoll round trip and
*        no funnelling of every connection's continuation through one shared thread ( see the lesson-learned
*        comment at the top of epollFastPathServer.c ). It is the wrong choice when there are far more
*        connections than there is work : a REST API behind thousands of keep-alive clients that each send a
*        request every few seconds would otherwise need one thread per mostly-idle connection. Here an idle
*        connection costs an fd and one struct ApiConnection , and the epoll round trip per request is noise
*        next to the request itself.
*
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef EPOLL_API_SERVER_H_INCLUDED
#define EPOLL_API_SERVER_H_INCLUDED

#include "../AmmServerlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Take ownership of a freshly accepted socket : make it non-blocking and register it with the accept
*        epoll layer as an API connection. Called from the accept path instead of dispatching to a worker
*        thread when the instance is in API mode.
* @ingroup threads
* @param An AmmarServer Instance
* @param The newly accepted client socket
* @retval 1=Adopted ( caller must not touch clientsock again ) , 0=Failed , caller should close it */
int EpollAPI_AdoptConnection(struct AmmServer_Instance * instance,int clientsock);

/**
* @brief Called by the accept epoll layer thread for an event whose data.ptr is an API connection : reads
*        whatever is available , answers every complete request in it through the instance's API handler and
*        either re-arms the connection for the next request or closes it.
* @ingroup threads
* @param An AmmarServer Instance
* @param The struct ApiConnection* carried in the epoll event's data.ptr
* @param The epoll event bits that were reported */
void EpollAPI_HandleEvent(struct AmmServer_Instance * instance,void * connection,unsigned int events);

#ifdef __cplusplus
}
#endif

#endif // EPOLL_API_SERVER_H_INCLUDED
