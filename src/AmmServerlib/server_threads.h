#ifndef SERVER_THREADS_H_INCLUDED
#define SERVER_THREADS_H_INCLUDED

#include "header_analysis/http_header_analysis.h"
#include "server_configuration.h"

void * ServeClient(void * ptr);

int StartHTTPServer(struct AmmServer_Instance * instance,char * ip,unsigned int port,char * root_path,char * templates_path);
int StopHTTPServer(struct AmmServer_Instance * instance);
int HTTPServerIsRunning(struct AmmServer_Instance * instance);

#endif // SERVER_THREADS_H_INCLUDED
