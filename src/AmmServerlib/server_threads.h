#ifndef SERVER_THREADS_H_INCLUDED
#define SERVER_THREADS_H_INCLUDED

#include "http_header_analysis.h"

extern int ACTIVE_CLIENT_THREADS;
extern struct HTTPRequest * http_requests_of_threads[MAX_CLIENT_THREADS];

int StartHTTPServer(char * ip,unsigned int port,char * root_path,char * templates_path);
int StopHTTPServer();
int HTTPServerIsRunning();

#endif // SERVER_THREADS_H_INCLUDED
