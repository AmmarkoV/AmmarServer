/** @file threadedServer.h
* @brief Creating new threads to serve clients , we only have one call that generates a thread that serves a client connection
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef THREADED_SERVER_H_INCLUDED
#define THREADED_SERVER_H_INCLUDED

#include "../header_analysis/http_header_analysis.h"
#include "../server_configuration.h"


/**
* @brief Main Call to Serve a client , this will in turn pick a prespawned thread or create a new one
* @ingroup threads
* @param PassToHTTPThread with information to pass to the new thread ( prespawned or not )
* @retval This function returns 0  */
void * ServeClient(void * ptr);


/**
* @brief Start HTTP server
* @ingroup threads
* @param An AmmarServer Instance
* @param String with the binding IP for the new server
* @param Port for binding the new server , ports under 1000 require super user privileges
* @param Filename to root path for this webserver ( public_html )
* @param Filename to root path for templates ( 404.html etc )
* @retval 1=Success,0=Failure  */
int StartHTTPServer(struct AmmServer_Instance * instance,char * ip,unsigned int port,char * root_path,char * templates_path);


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

#endif // SERVER_THREADS_H_INCLUDED
