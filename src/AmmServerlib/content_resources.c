/*
AmmarServer , HTTP Server Library

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>

#include "configuration.h"
#include "content_resources.h"
#include "network.h"


unsigned long SendBanner(int clientsock)
{
  char reply_header[1024]={0};
  char reply_body[1024]={0};
  char body_type[1024]={0};


  strcpy(body_type,"text/html");

  strcpy(reply_body,(char*) "<html><head><title>AmmarServer</title></head><body><center>");
  strcat(reply_body,(char*) "<br><br><br><h2><img src=\"up.gif\"><h2><h4> </h4>");
  strcat(reply_body,(char*) "</center></body></html>");



  sprintf(reply_header,"HTTP/1.1 200 OK\nServer: Ammarserver/%s\nConnection: close\nContent-type: %s\nContent-length: %u\n\n",AmmServerVERSION,body_type,(unsigned int) strlen(reply_body));
  //Date: day day month year hour:minute:second\n
  //Last-modified: day day month year hour:minute:second\n



  int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);
  if (opres<=0) { fprintf(stderr,"Error sending banner header \n"); return 0; }

      opres=send(clientsock,reply_body,strlen(reply_body),MSG_WAITALL);
  if (opres<=0) { fprintf(stderr,"Error sending banner body\n"); return 0; }

   return 1;
}

/*
  TODO : possibly add templates images/pages stuff here as a static resource..
  it will increase the executable size though and I dont really like that..!

 */
