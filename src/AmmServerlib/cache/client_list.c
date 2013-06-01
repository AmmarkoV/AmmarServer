#include "client_list.h"
#include "../hashmap/hashmap.h"

#include <stdio.h>

#define COMPILE_WITH_CLIENT_LIST 1
struct hashMap * userList=0;

unsigned int clientList_GetClientId(char * ip)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"GetClientId(%s) not implemented\n",ip);
   return 0;
  #else
   return 0;
  #endif
}

int clientList_isClientBanned(clientID client_id)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"ClientIsBanned(%u) not implemented\n",client_id);
   return 0;
  #else
   return 0;
  #endif
}

int clientList_isClientAllowedToUseResource(clientID client_id,char * resource)
{
  #if COMPILE_WITH_CLIENT_LIST
  fprintf(stderr,"AllowClientToUseResource(%u,%s) not implemented\n",client_id,resource);
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}


int clientList_signalClientStoppedUsingResource(clientID client_id,char * resource)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"ClientStoppedUsingResource(%u,%s) not implemented\n",client_id,resource);
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}


int clientList_initialize()
{
  #if COMPILE_WITH_CLIENT_LIST
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}


int clientList_close()
{
  #if COMPILE_WITH_CLIENT_LIST
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}

