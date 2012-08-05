#ifndef SERVER_THREADS_H_INCLUDED
#define SERVER_THREADS_H_INCLUDED


extern int server_running;
extern int pause_server;
extern int stop_server;


int StartHTTPServer(char * ip,unsigned int port,char * root_path,char * templates_path);
int StopHTTPServer();

#endif // SERVER_THREADS_H_INCLUDED
