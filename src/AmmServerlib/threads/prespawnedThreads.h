/** @file prespawnedThreads.h
* @brief Using already created threads to serve clients , we have a pool of threads that can be used to serve connections
* @author Ammar Qammaz (AmmarkoV)
* @bug Prespawned threads have race conditions ?
*/
#ifndef PRESPAWNEDTHREADS_H_INCLUDED
#define PRESPAWNEDTHREADS_H_INCLUDED

#include <pthread.h>
#include <netinet/in.h>
#include "../server_configuration.h"

/** @brief A structure that holds information to be passed from the main thread to the new (prespawned) thread */
struct PreSpawnedThread
{
    volatile unsigned char busy;

    unsigned int threadNum;

    struct AmmServer_Instance * instance;
    pthread_t thread_id;

    int clientsock;
    struct sockaddr_in client;
    unsigned int clientlen;

    //pthread_mutex_t operation_mutex;
	//pthread_cond_t  condition_var;

    char webserver_root[MAX_FILE_PATH];
    char templates_root[MAX_FILE_PATH];
};


/**
* @brief Create an initial pool of PreSpawned Threads , before handling any connections to be ready when a connection arrives
* @ingroup threads
* @param An AmmarServer Instance
* @retval 1=Success,0=Fail */
void PreSpawnThreads(struct AmmServer_Instance * instance);

/**
* @brief Use a PreSpawned Thread that will serve the incoming client socket connection
* @ingroup threads
* @param An AmmarServer Instance
* @param Client socket to be read
* @param Client socket to be read ( sockaddr_in )
* @param Length of client
* @param Filename of root directory for this connection ( public_html )
* @param Filename of template directory for this connection ( for 404.html etc )
* @retval 1=Success,0=Fail */
int UsePreSpawnedThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root);

#endif // PRESPAWNEDTHREADS_H_INCLUDED
