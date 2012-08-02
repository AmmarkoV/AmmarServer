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

#include <unistd.h>
#include <pthread.h>

#include "network.h"
#include "httpprotocol.h"
#include "httprules.h"
#include "file_caching.h"



const char * AmmServerVERSION="0.1";

int serversock;
int server_running=0;
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

char FileExists(char * filename)
{
 FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 return 0;
}




unsigned long SendErrorCodeHeader(int clientsock,unsigned int error_code,char * verified_filename)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/

     char response[512]={0};
     switch (error_code)
     {
       case 404 : strcpy(response,"404 Not Found"); strcpy(verified_filename,"public_html/templates/404.html"); break;
       case 400 : strcpy(response,"400 Bad Request"); strcpy(verified_filename,"public_html/templates/400.html"); break;
       case 500 : strcpy(response,"500 Internal Server Error"); strcpy(verified_filename,"public_html/templates/500.html"); break;
       //---------------------------------------------------------------------------------------------------------------------------
       default : strcpy(response,"500 Internal Server Error");  strcpy(verified_filename,"public_html/templates/500.html");  break;
     };


     char reply_header[512]={0};
     sprintf(reply_header,"HTTP/1.1 %s\nServer: Ammarserver/%s\nContent-type: text/html\n",response,AmmServerVERSION);
     int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL); //Send preliminary header to minimize lag
     if (opres<=0) { return 0; }

     return 1;
}







unsigned long SendFile(int clientsock,char * verified_filename,unsigned long start_at_byte,unsigned char header_only,unsigned char keepalive)
{
  char reply_header[1024]={0};

  char content_type[512]={0};
  strcpy(content_type,"text/html"); //image/gif;


  if (!FilenameStripperOk(verified_filename))
  {
     //Unsafe filename , bad request :P
     SendErrorCodeHeader(clientsock,400,verified_filename);
     //verified_filename should now point to the template file for 400 messages
  } else
  if (!FileExists(verified_filename))
   {
     //File doesnt exist , 404 error :P
     fprintf(stderr,"File Requested (%s) does not exist \n",verified_filename);
     SendErrorCodeHeader(clientsock,404,verified_filename);
     //verified_filename should now point to the template file for 404 messages
   } else
   {
      fprintf(stderr,"Sending File %s with response code 200 OK\n",verified_filename);
      GetContentType(verified_filename,content_type);
      sprintf(reply_header,"HTTP/1.1 200 OK\nServer: Ammarserver/%s\nContent-type: %s\n",AmmServerVERSION,content_type);
   }

  if (keepalive) { strcat(reply_header,"Connection: keep-alive\n"); } else
                 { strcat(reply_header,"Connection: close\n"); } //Append Keep-Alive or Close and then..
  int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL); //.. send preliminary header to minimize lag
  if (opres<=0) { fprintf(stderr,"Failed while sending header\n"); return 0; }


  unsigned long cached_lSize=0;
  char * cached_buffer = CheckForCachedVersionOfThePage(verified_filename,&cached_lSize);

  if ((cached_buffer!=0)&&(cached_lSize!=0))
   { /*TODO : Serve cached file */
     sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) cached_lSize);
     opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);  //Send filesize as soon as we've got it
     if (!header_only)
      {
       opres=send(clientsock,cached_buffer,cached_lSize,MSG_WAITALL);  //Send file as soon as we've got it
      }
     return opres;
   } else

  {
    /*TODO : Serve file by reading it from disk */
    if ((cached_buffer==0)&&(cached_lSize==1)) { /*TODO : Cache indicates that file doesn't exist */ } else
    if ((cached_buffer==0)&&(cached_lSize==0)) { /*TODO : Cache indicates that file is not in cache :P */ }



    FILE * pFile;
    unsigned long lSize;
    char * buffer;
    size_t result;

    pFile = fopen (verified_filename, "rb" );
    if (pFile==0) { fprintf(stderr,"Could not open file %s\n",verified_filename); return 0;}

    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) lSize);
    opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);  //Send filesize as soon as we've got it


    if (!header_only)
    {
      rewind (pFile);
      if (start_at_byte!=0) { fseek (pFile , start_at_byte , SEEK_SET); }

      // allocate memory to contain the whole file:
      //Todo make a smaller allocation and gradually serve the whole file :P
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
  }

    return opres;
  }
 return 0;
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

  -----------------------------------------------------------------
  -----------------------------------------------------------------
          /\                                             /\
          ||        FILE TRANSFER / HEADER CALLS         ||
          ||                                             ||
  -----------------------------------------------------------------
  -----------------------------------------------------------------
  -----------------------------------------------------------------
  -----------------------------------------------------------------
          ||                                             ||
          ||           PER CLIENT SERVING THREAD         ||
          \/                                             \/
  -----------------------------------------------------------------
  -----------------------------------------------------------------

*/

void * ServeClient(void * ptr)
{
  fprintf(stderr,"New Serve Client call\n");
  struct PassToHTTPThread * context = (struct PassToHTTPThread *) ptr;

  int clientsock=context->clientsock;
//  struct sockaddr_in client=context->client;
//  unsigned int clientlen=context->clientlen;
  context->keep_var_on_stack=2;


  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  if (setsockopt (clientsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Receive timeout \n"); }
  if (setsockopt (clientsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Send timeout \n"); }


  char incoming_request[4096]; //A 4K header is more than enough..!

  int close_connection=0;

  while ( (!close_connection)&&(server_running) )
  {
   incoming_request[0]=0;
   int total_header=0,opres=0;
   fprintf(stderr,"Waiting for a valid HTTP header..\n");
   while ( (!HTTPRequestComplete(incoming_request,total_header))&&(server_running) )
   { //Gather Header until http request contains two newlines..!
     opres=recv(clientsock,&incoming_request[total_header],4096-total_header,0);
     if (opres<=0)
      {
        //TODO : Check opres here..!
        close_connection=1;
        break;
      } else
      {
       fprintf(stderr,"Got %u bytes \n",opres);
       total_header+=opres;
      }
   }


  if (opres>0)
  {
   fprintf(stderr,"Received %s \n",incoming_request);
   struct HTTPRequest output;
   memset(&output,0,sizeof(struct HTTPRequest));

   int result = AnalyzeHTTPRequest(&output,incoming_request,total_header);

   if (!result) {  /*We got a bad http request so we will rig it to make server emmit the 500 message*/
                   strcpy(output.resource,"/...../root/bad//GET/500/CODE:P\0\1\2\3$$#:P"); }

   if (!output.keepalive) { close_connection=1; }

   if ( (output.requestType==GET)||(output.requestType==HEAD))
   {
      char servefile[512]={0};
      if (strcmp(output.resource,"/")==0) { strcpy(servefile,"public_html/index.html"); } else
                                          { strcpy(servefile,"public_html/"); strcat(servefile,output.resource); }



      //SendFile decides about the safety of the resource requested..
      //it should deny requests to paths like ../ or /etc/passwd
      SendFile(clientsock,servefile,0,(output.requestType==HEAD),1);
   }
  }

  }
  close(clientsock);
  pthread_exit(0);
  return 0;
}



int SpawnThreadToServeNewClient(int clientsock,struct sockaddr_in client,unsigned int clientlen)
{
  fprintf(stderr,"Server Thread : Client connected: %s\n", inet_ntoa(client.sin_addr));

  struct PassToHTTPThread context;
   memset(&context,0,sizeof(struct PassToHTTPThread));

  context.clientsock=clientsock;
  context.client=client;
  context.clientlen=clientlen;
  context.keep_var_on_stack=1;


  int retres = pthread_create(&threads_pool[ACTIVE_CLIENT_THREADS++],0,ServeClient,(void*) &context);
  if ( retres==0 ) { while (context.keep_var_on_stack==1) { usleep(10); } } // <- Keep PeerServerContext in stack for long enough :P


  if (retres!=0) retres = 0; else
                 retres = 1;

  return retres;
}

/*

  -----------------------------------------------------------------
  -----------------------------------------------------------------
          /\                                             /\
          ||           PER CLIENT SERVING THREAD         ||
          ||                                             ||
  -----------------------------------------------------------------
  -----------------------------------------------------------------
  -----------------------------------------------------------------
  -----------------------------------------------------------------
          ||                                             ||
          ||            MAIN HTTP SERVER THREAD          ||
          \/                                             \/
  -----------------------------------------------------------------
  -----------------------------------------------------------------

*/




void * HTTPServerThread (void * ptr)
{

  struct PassToHTTPThread * context = (struct PassToHTTPThread *) ptr;


  int clientsock;
  unsigned int serverlen = sizeof(struct sockaddr_in),clientlen = sizeof(struct sockaddr_in);
  struct sockaddr_in server;
  struct sockaddr_in client;

  serversock = socket(AF_INET, SOCK_STREAM, 0);
    if ( serversock < 0 ) { error("Server Thread : Opening socket"); server_running=0; return 0; }


  bzero(&client,clientlen);
  bzero(&server,serverlen);

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(context->port);

  context->keep_var_on_stack=2;

  if ( bind(serversock,(struct sockaddr *) &server,serverlen) < 0 ) { error("Server Thread : Error binding master port!"); server_running=0; return 0; }
  if (listen(serversock,10) < 0)  { error("Server Thread : Failed to listen on server socket"); server_running=0; return 0; }


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
  server_running=0;
  stop_server=2;

  //It should already be closed so skipping this : close(serversock);
  pthread_exit(0);
  return 0;
}



int StartHTTPServer(char * ip,unsigned int port)
{
  struct PassToHTTPThread context;
  memset(&context,0,sizeof(context));

  strncpy(context.ip,ip,255);
  context.port=port;
  context.keep_var_on_stack=1;

  server_running=1;
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

int StopHTTPServer()
{
  /*
     We want to stop the server that accepts new connections ( and we do that by signaling stop_server=1;
     The problem is that it will keep waiting for one more job since it is blocked in the accept call..!
     Thats why we force the socket close which in turn terminates the server thread..
  */
  if ( (stop_server==2)||(stop_server==0)) { fprintf(stderr,"Server has stopped working on its own..\n"); return 1;}

  stop_server=1;
  fprintf(stderr,"Waiting for Server to stop.. ");
  close(serversock);
  while (stop_server!=2)
    {
        fprintf(stderr,".");
        usleep(10000);
    }
  fprintf(stderr," .. done \n");

  return (stop_server==2);
}
