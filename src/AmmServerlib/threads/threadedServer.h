/** @file threadedServer.h
* @brief Creating new threads to serve clients , we only have one call that generates a thread that serves a client connection
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef THREADED_SERVER_H_INCLUDED
#define THREADED_SERVER_H_INCLUDED

#include "../header_analysis/http_header_analysis.h"
#include "../server_configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

//Discriminator tag placed as the first field of every heap struct handed to epoll via event.data.ptr on
//instance->accept_epoll_fd , so EpollAcceptLayerThread ( threadedServer.c ) knows how to interpret an event
//before casting the pointer - shared with epollFastPathServer.c , which allocates EPOLL_EVENT_KIND_FASTPATH_WRITE.
enum EpollEventKind
{
   EPOLL_EVENT_KIND_PENDING_ACCEPT = 1 ,
   EPOLL_EVENT_KIND_FASTPATH_WRITE = 2
};

/**
* @brief Start HTTP server
* @ingroup threads
* @param An AmmarServer Instance
* @param String with the binding IP for the new server
* @param Port for binding the new server , ports under 1000 require super user privileges
* @param Filename to root path for this webserver ( public_html )
* @param Filename to root path for templates ( 404.html etc )
* @retval 1=Success,0=Failure  */
int StartThreadedHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path);

/**
* @brief Stop a running HTTP server , unbind ports , deallocate structures etc
* @ingroup threads
* @param An AmmarServer Instance
* @bug Stop web server should be improved , to make sure it unbinds the closing socket
* @retval 1=Success,0=Failure  */
int StopThreadedHTTPServer(struct AmmServer_Instance * instance);

/**
* @brief Ask if the HTTP server is running
* @ingroup threads
* @param An AmmarServer Instance
* @retval 1=Success,0=Failure  */
int ThreadedHTTPServerIsRunning(struct AmmServer_Instance * instance);


/**
* @brief Ask about the number of threads running on the background
* @ingroup threads
* @param An AmmarServer Instance
* @retval Total number of running threads  */
unsigned int GetActiveThreadedHTTPServerThreads(struct AmmServer_Instance * instance);

#ifdef __cplusplus
}
#endif

#endif // SERVER_THREADS_H_INCLUDED
