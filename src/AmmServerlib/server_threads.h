#ifndef SERVER_THREADS_H_INCLUDED
#define SERVER_THREADS_H_INCLUDED


int StartHTTPServer(char * ip,unsigned int port,char * root_path,char * templates_path);
int StopHTTPServer();
int HTTPServerIsRunning();

#endif // SERVER_THREADS_H_INCLUDED
