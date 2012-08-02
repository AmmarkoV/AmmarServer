#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED


extern int server_running;
extern int pause_server;
extern int stop_server;


int StartHTTPServer(char * ip,unsigned int port);
int StopHTTPServer();

#endif // NETWORK_H_INCLUDED
