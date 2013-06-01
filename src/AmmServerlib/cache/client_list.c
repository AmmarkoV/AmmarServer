#include "client_list.h"

#include <stdio.h>

#define COMPILE_WITH_CLIENT_LIST 1

unsigned int clientList_GetClientId(struct clientListContext * clientList,char * ip)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"GetClientId(%s) not implemented\n",ip);
   return 0;
  #else
   return 0;
  #endif
}

int clientList_isClientBanned(struct clientListContext * clientList,clientID client_id)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"ClientIsBanned(%u) not implemented\n",client_id);
   return 0;
  #else
   return 0;
  #endif
}

int clientList_isClientAllowedToUseResource(struct clientListContext * clientList,clientID client_id,char * resource)
{
  #if COMPILE_WITH_CLIENT_LIST
  fprintf(stderr,"AllowClientToUseResource(%u,%s) not implemented\n",client_id,resource);
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}


int clientList_signalClientStoppedUsingResource(struct clientListContext * clientList,clientID client_id,char * resource)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"ClientStoppedUsingResource(%u,%s) not implemented\n",client_id,resource);
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}


struct clientListContext *  clientList_initialize()
{
  #if COMPILE_WITH_CLIENT_LIST
   return 0;
  #else
   return 0;
  #endif // COMPILE_WITH_CLIENT_LIST
}


int clientList_close(struct clientListContext * clientList)
{
  #if COMPILE_WITH_CLIENT_LIST
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}

