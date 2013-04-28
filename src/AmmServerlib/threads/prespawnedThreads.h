#ifndef PRESPAWNEDTHREADS_H_INCLUDED
#define PRESPAWNEDTHREADS_H_INCLUDED

#include <pthread.h>
#include <netinet/in.h>
#include "../server_configuration.h"

struct PreSpawnedThread
{
    unsigned int threadNum;

    struct AmmServer_Instance * instance;
    pthread_t thread_id;

    int clientsock;
    struct sockaddr_in client;
    unsigned int clientlen;

    volatile unsigned char busy;

    //pthread_mutex_t operation_mutex;
	//pthread_cond_t  condition_var;

    char webserver_root[MAX_FILE_PATH];
    char templates_root[MAX_FILE_PATH];
};


#endif // PRESPAWNEDTHREADS_H_INCLUDED
