#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>

#include <unistd.h>
#include <pthread.h>

int pause_server=0;
int stop_server=0;
pthread_t server_thread_id;


#define MAX_CLIENT_THREADS 255
int ACTIVE_CLIENT_THREADS=0;
pthread_t threads_pool[MAX_CLIENT_THREADS];


struct PassToHTTPThread
{
     char ip[256];
     unsigned int port;
     unsigned int keep_var_on_stack;

     int clientsock;
     struct sockaddr_in client;
     unsigned int clientlen;
};


void * ServeClient(void * ptr);
void * HTTPServerThread (void * ptr);

void error(char * msg)
{
 fprintf(stderr,"ERROR MESSAGE : %s\n",msg);
 return;
}


unsigned long SendFile(int clientsock,char * verified_filename,char * content_type,unsigned long start_at_byte)
{

  char reply_header[1024]={0};
  sprintf(reply_header,"HTTP/1.1 200 OK\nServer: Ammarserver/0.0\nConnection: close\nContent-type: %s\n",content_type);
  int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL); //Send preliminary header to minimize lag


  FILE * pFile;
  unsigned long lSize;
  char * buffer;
  size_t result;

  pFile = fopen (verified_filename, "rb" );
  if (pFile==0) { fprintf(stderr,"Could not opeen file %s\n",verified_filename); return 0;}

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  lSize = ftell (pFile);
  sprintf(reply_header,"Content-length: %u\n",lSize);
  opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);  //Send filesize as soon as we've got it

  rewind (pFile);

  if (start_at_byte!=0) { fseek (pFile , start_at_byte , SEEK_SET); }

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == 0) { fprintf(stderr," Could not allocate enough memory to serve file %s\n",verified_filename); return 0;}

  // copy the file into the buffer:
  result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); return 0;}


  opres=send(clientsock,buffer,result,MSG_WAITALL);  //Send file as soon as we've got it
  /* the whole file is now loaded in the memory buffer. */

  // terminate
  fclose (pFile);
  free (buffer);

  return lSize;

}



unsigned long SendBanner(int clientsock)
{
  char reply_header[1024]={0};
  char reply_body[1024]={0};
  char body_type[1024]={0};


  strcpy(body_type,"text/html");

  strcpy(reply_body,(char*) "<html><head><title>AmmarServer</title></head><body><center>");
  strcat(reply_body,(char*) "<br><br><br><h2><img src=\"up.gif\"><h2><h4> </h4>");
  strcat(reply_body,(char*) "</center></body></html>");



  sprintf(reply_header,"HTTP/1.1 200 OK\nServer: Ammarserver/0.0\nConnection: close\nContent-type: %s\nContent-length: %u\n\n",body_type,strlen(reply_body));
  //Date: day day month year hour:minute:second\n
  //Last-modified: day day month year hour:minute:second\n



  int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);
      opres=send(clientsock,reply_body,strlen(reply_body),MSG_WAITALL);
}




void * ServeClient(void * ptr)
{
  struct PassToHTTPThread * context = (struct PassToHTTPThread *) ptr;

  int clientsock=context->clientsock;
  struct sockaddr_in client=context->client;
  unsigned int clientlen=context->clientlen;
  context->keep_var_on_stack=2;

  char incoming_request[2048];
  int opres=recv(clientsock,&incoming_request,2048,0);
  fprintf(stderr,"Received %s \n",incoming_request);

  //SendBanner(clientsock);
  SendFile(clientsock,"public_html/up.gif","image/gif",0);


  close(clientsock);
  pthread_exit(0);
  return 0;
}



int SpawnThreadToServeNewClient(int clientsock,struct sockaddr_in client,unsigned int clientlen)
{
  fprintf(stderr,"Server Thread : Client connected: %s\n", inet_ntoa(client.sin_addr));

  struct PassToHTTPThread context={0};
  context.clientsock=clientsock;
  context.client=client;
  context.clientlen=clientlen;


  int retres = pthread_create(&threads_pool[ACTIVE_CLIENT_THREADS++],0,ServeClient,(void*) &context);
  if ( retres==0 ) { while (context.keep_var_on_stack==1) { usleep(10); } } // <- Keep PeerServerContext in stack for long enough :P


  fprintf(stderr,"done\n");

  if (retres!=0) retres = 0; else
                 retres = 1;

  return retres;
}





void * HTTPServerThread (void * ptr)
{

  struct PassToHTTPThread * context = (struct PassToHTTPThread *) ptr;


  int serversock,clientsock;
  unsigned int serverlen = sizeof(struct sockaddr_in),clientlen = sizeof(struct sockaddr_in);
  struct sockaddr_in server;
  struct sockaddr_in client;

  serversock = socket(AF_INET, SOCK_STREAM, 0);
    if ( serversock < 0 ) { error("Server Thread : Opening socket"); return 0; }


  bzero(&client,clientlen);
  bzero(&server,serverlen);

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(context->port);

  context->keep_var_on_stack=2;

  if ( bind(serversock,(struct sockaddr *) &server,serverlen) < 0 ) { error("Server Thread : Error binding master port!"); return 0; }
  if (listen(serversock,10) < 0)  { error("Server Thread : Failed to listen on server socket"); return 0; }


  while (stop_server==0)
  {
    printf("Server Thread : Waiting for a new client");
    /* Wait for client connection */
    if ( (clientsock = accept(serversock,(struct sockaddr *) &client, &clientlen)) < 0) { error("Server Thread : Failed to accept client connection"); }
      else
      {
           fprintf(stderr,"Server Thread : Accepted new client \n");
           if (!SpawnThreadToServeNewClient(clientsock,client,clientlen))
            {
                fprintf(stderr,"Server Thread : Client failed, while handling him\n");
                close(clientsock);
            }
      }
 }
  return 0;
}



int StartHTTPServer(char * ip,unsigned int port)
{
  struct PassToHTTPThread context={0};
  strncpy(context.ip,ip,255);
  context.port=port;
  context.keep_var_on_stack=1;

  pause_server=0;
  stop_server=0;

  int retres=1;

  retres = pthread_create( &server_thread_id ,0,HTTPServerThread,(void*) &context);
  if (retres!=0) retres = 0; else
                 retres = 1;

  while (context.keep_var_on_stack==1)
   {
      printf("..");
      usleep(1000);
     // wait;
   }

  return retres;
}


