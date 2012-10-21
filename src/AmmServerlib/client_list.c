#include "client_list.h"
#include <stdio.h>

unsigned int GetClientId(char * ip)
{
  //fprintf(stderr,"GetClientId(%s) not implemented\n",ip);
  return 0;
}


int ClientIsBanned(unsigned int client_id)
{
  //fprintf(stderr,"ClientIsBanned(%u) not implemented\n",client_id);
  return 0;
}


int AllowClientToUseResource(unsigned int client_id,char * resource)
{
  //fprintf(stderr,"AllowClientToUseResource(%u,%s) not implemented\n",client_id,resource);
  return 1;
}


int ClientStoppedUsingResource(unsigned int client_id,char * resource)
{
  //fprintf(stderr,"ClientStoppedUsingResource(%u,%s) not implemented\n",client_id,resource);
  return 1;
}
