#ifndef CLIENT_LIST_H_INCLUDED
#define CLIENT_LIST_H_INCLUDED

typedef unsigned int clientID;


unsigned int clientList_GetClientId(char * ip);
int clientList_isClientBanned(clientID client_id);
int clientList_isClientAllowedToUseResource(clientID client_id,char * resource);
int clientList_signalClientStoppedUsingResource(clientID client_id,char * resource);

int clientList_initialize();
int clientList_close();
#endif // CLIENT_LIST_H_INCLUDED
