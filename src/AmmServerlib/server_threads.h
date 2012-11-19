#ifndef SERVER_THREADS_H_INCLUDED
#define SERVER_THREADS_H_INCLUDED

#include "http_header_analysis.h"
#include "server_configuration.h"

extern int CLIENT_THREADS_STARTED;
extern int CLIENT_THREADS_STOPPED;
extern struct HTTPRequest * http_requests_of_threads[MAX_CLIENT_THREADS];


int RegisterNewTransactionID(unsigned int already_have_an_id,struct HTTPRequest * the_transaction);
int FreeTransactionID(unsigned int transaction_id);
struct HTTPRequest * GetRequestStructForTransactionID(unsigned int transaction_id);

int StartHTTPServer(char * ip,unsigned int port,char * root_path,char * templates_path);
int StopHTTPServer();
int HTTPServerIsRunning();

#endif // SERVER_THREADS_H_INCLUDED
