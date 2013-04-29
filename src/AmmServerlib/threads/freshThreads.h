#ifndef FRESHTHREADS_H_INCLUDED
#define FRESHTHREADS_H_INCLUDED

#include <netinet/in.h>
#include "../server_configuration.h"


struct PassToHTTPThread
{
     volatile int keep_var_on_stack;
     struct AmmServer_Instance * instance;

     char ip[MAX_IP_STRING_SIZE];
     char webserver_root[MAX_FILE_PATH];
     char templates_root[MAX_FILE_PATH];

     unsigned int port;
     int pre_spawned_thread;

     int clientsock;
     struct sockaddr_in client;
     unsigned int clientlen;
     unsigned int thread_id;
};

int SpawnThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root);

#endif // FRESHTHREADS_H_INCLUDED
