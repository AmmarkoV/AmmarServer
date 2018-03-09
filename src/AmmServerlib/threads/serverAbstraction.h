/** @file serverAbstraction.h
* @brief The way to switch from Pthread to libEvent threading
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef SERVERABSTRACTION_H_INCLUDED
#define SERVERABSTRACTION_H_INCLUDED

#include "../AmmServerlib.h"
#include <stdlib.h>


/**
* @brief Start HTTP server
* @ingroup threads
* @param An AmmarServer Instance
* @param String with the binding IP for the new server
* @param Port for binding the new server , ports under 1000 require super user privileges
* @param Filename to root path for this webserver ( public_html )
* @param Filename to root path for templates ( 404.html etc )
* @retval 1=Success,0=Failure  */
int ASRV_StartHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path);

/**
* @brief Stop a running HTTP server , unbind ports , deallocate structures etc
* @ingroup threads
* @param An AmmarServer Instance
* @bug Stop web server should be improved , to make sure it unbinds the closing socket
* @retval 1=Success,0=Failure  */
int ASRV_StopHTTPServer(struct AmmServer_Instance * instance);

/**
* @brief Ask if the HTTP server is running
* @ingroup threads
* @param An AmmarServer Instance
* @retval 1=Success,0=Failure  */
int ASRV_HTTPServerIsRunning(struct AmmServer_Instance * instance);


/**
* @brief Ask about the number of threads running on the background
* @ingroup threads
* @param An AmmarServer Instance
* @retval Total number of running threads  */
unsigned int ASRV_GetActiveHTTPServerThreads(struct AmmServer_Instance * instance);

#endif
