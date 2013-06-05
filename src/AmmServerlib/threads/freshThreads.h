#ifndef FRESHTHREADS_H_INCLUDED
#define FRESHTHREADS_H_INCLUDED

#include <netinet/in.h>
#include "../server_configuration.h"


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
};

int SpawnThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root);

#endif // FRESHTHREADS_H_INCLUDED
