#ifndef CLIENT_LIST_H_INCLUDED
#define CLIENT_LIST_H_INCLUDED

unsigned int GetClientId(char * ip);
int ClientIsBanned(unsigned int client_id);
int AllowClientToUseResource(unsigned int client_id,char * resource);
int ClientStoppedUsingResource(unsigned int client_id,char * resource);

#endif // CLIENT_LIST_H_INCLUDED
