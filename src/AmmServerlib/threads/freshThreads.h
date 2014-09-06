/** @file freshThreads.h
* @brief Creating new threads to serve clients , we only have one call that generates a thread that serves a client connection
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef FRESHTHREADS_H_INCLUDED
#define FRESHTHREADS_H_INCLUDED

#include <netinet/in.h>
#include "../server_configuration.h"

/** @brief A structure that holds information to be passed from the main thread to the new (fresh) thread */
struct PassToHTTPThread
{
     volatile int keep_var_on_stack;

     struct sockaddr_in client;
     unsigned int clientlen;
     unsigned int thread_id;
     unsigned int port;

     int clientsock;
     struct AmmServer_Instance * instance;

     int pre_spawned_thread;

     char ip[MAX_IP_STRING_SIZE];
};

/**
* @brief Create a new Thread that will serve the incoming client socket connection
* @ingroup threads
* @param An AmmarServer Instance
* @param Client socket to be read
* @param Client socket to be read ( sockaddr_in )
* @param Length of client
* @param Filename of root directory for this connection ( public_html )
* @param Filename of template directory for this connection ( for 404.html etc )
* @retval 1=Success,0=Fail
*/
int SpawnThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen);

#endif // FRESHTHREADS_H_INCLUDED
