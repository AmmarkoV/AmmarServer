#ifndef SERVER_THREADS_H_INCLUDED
#define SERVER_THREADS_H_INCLUDED

#include <netinet/in.h>

#include "http_header_analysis.h"
#include "server_configuration.h"

extern int CLIENT_THREADS_STARTED;
extern int CLIENT_THREADS_STOPPED;
extern struct HTTPRequest * http_requests_of_threads[MAX_CLIENT_THREADS];
extern unsigned int GLOBAL_KILL_SERVER_SWITCH;

struct PreSpawnedThread
{
    struct AmmServer_Instance * instance;
    pthread_t thread_id;

    int clientsock;
    struct sockaddr_in client;
    unsigned int clientlen;

    char busy;

    char webserver_root[MAX_FILE_PATH];
    char templates_root[MAX_FILE_PATH];
};

int RegisterNewTransactionID(unsigned int already_have_an_id,struct HTTPRequest * the_transaction);
int FreeTransactionID(unsigned int transaction_id);
struct HTTPRequest * GetRequestStructForTransactionID(unsigned int transaction_id);

int StartHTTPServer(struct AmmServer_Instance * instance,char * ip,unsigned int port,char * root_path,char * templates_path);
int StopHTTPServer(struct AmmServer_Instance * instance);
int HTTPServerIsRunning(struct AmmServer_Instance * instance);

#endif // SERVER_THREADS_H_INCLUDED
