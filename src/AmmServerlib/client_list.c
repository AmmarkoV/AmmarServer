#include "client_list.h"


unsigned int GetClientId(char * ip)
{
  return 0;
}


int ClientIsBanned(unsigned int client_id)
{
  return 0;
}


int AllowClientToUseResource(unsigned int client_id,char * resource)
{
  return 1;
}


int ClientStoppedUsingResource(unsigned int client_id,char * resource)
{
  return 1;
}
