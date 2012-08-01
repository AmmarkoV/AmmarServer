#include <stdio.h>
#include <stdlib.h>
#include "AmmServerlib.h"
#include "network.h"



int AmmServer_Start(char * ip,unsigned int port)
{
  return StartHTTPServer(ip,port);
}

int AmmServer_Stop()
{
  return 0;
}


int AmmServer_Running()
{
  return server_running;
}
