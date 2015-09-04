/** @file threadedServer.h
* @brief Creating new threads to serve clients , we only have one call that generates a thread that serves a client connection
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef THREADED_SERVER_H_INCLUDED
#define THREADED_SERVER_H_INCLUDED

#include "../header_analysis/http_header_analysis.h"
#include "../server_configuration.h"

/**
* @brief Start HTTP server
* @ingroup threads
* @param An AmmarServer Instance
* @param String with the binding IP for the new server
* @param Port for binding the new server , ports under 1000 require super user privileges
* @param Filename to root path for this webserver ( public_html )
* @param Filename to root path for templates ( 404.html etc )
* @retval 1=Success,0=Failure  */
int StartHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path);

/**
* @brief Stop a running HTTP server , unbind ports , deallocate structures etc
* @ingroup threads
* @param An AmmarServer Instance
* @bug Stop web server should be improved , to make sure it unbinds the closing socket
* @retval 1=Success,0=Failure  */
int StopHTTPServer(struct AmmServer_Instance * instance);

/**
* @brief Ask if the HTTP server is running
* @ingroup threads
* @param An AmmarServer Instance
* @retval 1=Success,0=Failure  */
int HTTPServerIsRunning(struct AmmServer_Instance * instance);


/**
* @brief Ask about the number of threads running on the background
* @ingroup threads
* @param An AmmarServer Instance
* @retval Total number of running threads  */
unsigned int GetActiveHTTPServerThreads(struct AmmServer_Instance * instance);
#endif // SERVER_THREADS_H_INCLUDED
