#include "client_list.h"
#include <stdio.h>

unsigned int GetClientId(char * ip)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"GetClientId(%s) not implemented\n",ip);
   return 0;
  #else
   return 0;
  #endif
}


int ClientIsBanned(unsigned int client_id)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"ClientIsBanned(%u) not implemented\n",client_id);
   return 0;
  #else
   return 0;
  #endif
}


int AllowClientToUseResource(unsigned int client_id,char * resource)
{
  #if COMPILE_WITH_CLIENT_LIST
  fprintf(stderr,"AllowClientToUseResource(%u,%s) not implemented\n",client_id,resource);
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}


int ClientStoppedUsingResource(unsigned int client_id,char * resource)
{
  #if COMPILE_WITH_CLIENT_LIST
   fprintf(stderr,"ClientStoppedUsingResource(%u,%s) not implemented\n",client_id,resource);
   return 1;
  #else
   return 1;
  #endif // COMPILE_WITH_CLIENT_LIST
}
