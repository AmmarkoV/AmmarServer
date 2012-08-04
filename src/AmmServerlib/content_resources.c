
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>

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
