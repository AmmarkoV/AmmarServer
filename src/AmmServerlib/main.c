/*  Copyright 2012 Ammar Qammaz
    This file is part of AmmarServer.
    AmmarServer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    AmmarServer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with AmmarServer. If not, see http://www.gnu.org/licenses/.*/
#include <stdio.h>
#include <stdlib.h>
#include "AmmServerlib.h"
#include "network.h"



int AmmServer_Start(char * ip,unsigned int port)
{
  fprintf(stderr,"\n\nDISCLAIMER : \n");
  fprintf(stderr,"Please note that this server version is not thoroughly\n");
  fprintf(stderr," pen-tested so it is not ready for production deployment..\n");

  fprintf(stderr,"Bug reports and feedback are very welcome.. \n");
  fprintf(stderr,"via https://github.com/AmmarkoV/AmmarServer/issues\n\n");

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
