/** @file libeventServer.h
* @brief The basic server threading done through libevent
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef LIBEVENT_SERVER_H_INCLUDED
#define LIBEVENT_SERVER_H_INCLUDED

//#define USE_LIBEVENT 0 1

#if USE_LIBEVENT

#ifdef __cplusplus
extern "C" {
#endif


#include "../AmmServerlib.h"

/**
* @brief Start HTTP server
* @ingroup threads
* @param An AmmarServer Instance
* @param String with the binding IP for the new server
* @param Port for binding the new server , ports under 1000 require super user privileges
* @param Filename to root path for this webserver ( public_html )
* @param Filename to root path for templates ( 404.html etc )
* @retval 1=Success,0=Failure  */
int StartLibEventHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path);

/**
* @brief Stop a running HTTP server , unbind ports , deallocate structures etc
* @ingroup threads
* @param An AmmarServer Instance
* @bug Stop web server should be improved , to make sure it unbinds the closing socket
* @retval 1=Success,0=Failure  */
int StopLibEventHTTPServer(struct AmmServer_Instance * instance);

/**
* @brief Ask if the HTTP server is running
* @ingroup threads
* @param An AmmarServer Instance
* @retval 1=Success,0=Failure  */
int LibEventHTTPServerIsRunning(struct AmmServer_Instance * instance);

/**
* @brief Ask about the number of threads running on the background
* @ingroup threads
* @param An AmmarServer Instance
* @retval Total number of running threads  */
unsigned int GetActiveLibEventHTTPServerThreads(struct AmmServer_Instance * instance)


#ifdef __cplusplus
}
#endif


#endif // USE_LIBEVENT


#endif // LIBEVENT_SERVER_H_INCLUDED
