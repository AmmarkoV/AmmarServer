#ifndef CLIENT_LIST_H_INCLUDED
#define CLIENT_LIST_H_INCLUDED


#include "../hashmap/hashmap.h"

typedef unsigned int clientID;

struct clientListContext
{
  struct hashMap * userList;
};

unsigned int clientList_GetClientId(struct clientListContext * clientList, char * ip);
int clientList_isClientBanned(struct clientListContext * clientList,clientID client_id);
int clientList_isClientAllowedToUseResource(struct clientListContext * clientList,clientID client_id,char * resource);
int clientList_signalClientStoppedUsingResource(struct clientListContext * clientList,clientID client_id,char * resource);

struct clientListContext *  clientList_initialize();
int clientList_close(struct clientListContext * clientList);
#endif // CLIENT_LIST_H_INCLUDED
