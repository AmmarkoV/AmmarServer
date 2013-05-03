#ifndef PRESPAWNEDTHREADS_H_INCLUDED
#define PRESPAWNEDTHREADS_H_INCLUDED

#include <pthread.h>
#include <netinet/in.h>
#include "../server_configuration.h"

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


void PreSpawnThreads(struct AmmServer_Instance * instance);
int UsePreSpawnedThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root);

#endif // PRESPAWNEDTHREADS_H_INCLUDED
