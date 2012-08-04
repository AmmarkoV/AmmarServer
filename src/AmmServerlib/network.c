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
#include "configuration.h"



const char * AmmServerVERSION="0.24";


int serversock;
int server_running=0;
int pause_server=0;
int stop_server=0;
pthread_t server_thread_id;

pthread_mutex_t thread_pool_access;
int ACTIVE_CLIENT_THREADS=0;
pthread_t threads_pool[MAX_CLIENT_THREADS]={0};


struct PassToHTTPThread
{
     char ip[256];
     char webserver_root[MAX_FILE_PATH];
     char templates_root[MAX_FILE_PATH];

     unsigned int port;
     unsigned int keep_var_on_stack;

     int clientsock;
     struct sockaddr_in client;
     unsigned int clientlen;
     unsigned int thread_id;
};


void * ServeClient(void * ptr);
void * HTTPServerThread (void * ptr);



unsigned long SendErrorCodeHeader(int clientsock,unsigned int error_code,char * verified_filename,char * templates_root)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/

     char response[512]={0};
     switch (error_code)
     {
       case 400 : strcpy(response,"400 Bad Request");            strcpy(verified_filename,templates_root); strcat(verified_filename,"400.html"); break;
       case 401 : strcpy(response,"401 Password Protected");     strcpy(verified_filename,templates_root); strcat(verified_filename,"401.html"); break;
       case 404 : strcpy(response,"404 Not Found");              strcpy(verified_filename,templates_root); strcat(verified_filename,"404.html"); break;
       case 408 : strcpy(response,"408 Timed Out");              strcpy(verified_filename,templates_root); strcat(verified_filename,"408.html"); break;
       case 500 : strcpy(response,"500 Internal Server Error");  strcpy(verified_filename,templates_root); strcat(verified_filename,"500.html"); break;
       case 501 : strcpy(response,"501 Not Implemented");        strcpy(verified_filename,templates_root); strcat(verified_filename,"501.html"); break;
       //---------------------------------------------------------------------------------------------------------------------------
       default : strcpy(response,"500 Internal Server Error");  strcpy(verified_filename,templates_root); strcat(verified_filename,"500.html"); break;
     };


     char reply_header[512]={0};
     sprintf(reply_header,"HTTP/1.1 %s\nServer: Ammarserver/%s\nContent-type: text/html\n",response,AmmServerVERSION);
     int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL); //Send preliminary header to minimize lag
     if (opres<=0) { return 0; }

     return 1;
}







unsigned long SendFile
  (
    int clientsock, // The socket that will be used to send the data
    char * verified_filename_pending_copy, // The filename to be served on the socket above

    unsigned long start_at_byte,   // Optionally start with an offset ( resume download functionality )
    unsigned int force_error_code, // Instead of the file , serve an error code..!
    unsigned char header_only,     // Only serve header ( HEAD instead of GET )
    unsigned char keepalive,       // Keep alive functionality
    unsigned char gzip_supported,  // If gzip is supported try to use it!

    //char * webserver_root,
    char * templates_root // In case we fail to serve verified_filename_etc.. serve something from the templates..!
    )
{
  char verified_filename[MAX_FILE_PATH]={0};
  strncpy(verified_filename,verified_filename_pending_copy,MAX_FILE_PATH);

  char reply_header[MAX_HTTP_RESPONSE_HEADER]={0};
  char content_type[MAX_CONTENT_TYPE]={0};

  strcpy(content_type,"text/html"); //image/gif;


  if (force_error_code!=0)
  {
    //We want to force a specific error_code!
    SendErrorCodeHeader(clientsock,force_error_code,verified_filename,templates_root);
  } else
  if (!FilenameStripperOk(verified_filename))
  {
     //Unsafe filename , bad request :P
     SendErrorCodeHeader(clientsock,400,verified_filename,templates_root);
     //verified_filename should now point to the template file for 400 messages
  } else
  if (!FileExists(verified_filename))
   {
     //File doesnt exist , 404 error :P
     fprintf(stderr,"File Requested (%s) does not exist \n",verified_filename);
     SendErrorCodeHeader(clientsock,404,verified_filename,templates_root);
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
  char * cached_buffer = CheckForCachedVersionOfThePage(verified_filename,&cached_lSize,gzip_supported);

  if ((cached_buffer!=0)&&(cached_lSize!=0))
   { /*!Serve cached file !*/
     if (gzip_supported) { strcat(reply_header,"Content-encoding: gzip\n"); } // Cache can serve gzipped files
     sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) cached_lSize);
     opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);  //Send filesize as soon as we've got it
     if (!header_only)
      {
       opres=send(clientsock,cached_buffer,cached_lSize,MSG_WAITALL);  //Send file as soon as we've got it
      }
     return opres;
   }
     else
  { /*!Serve file by reading it from disk !*/
    if ((cached_buffer==0)&&(cached_lSize==1)) { /*TODO : Cache indicates that file doesn't exist */ } else
    if ((cached_buffer==0)&&(cached_lSize==0)) { /*TODO : Cache indicates that file is not in cache :P */ }

    FILE * pFile=0;
    unsigned long lSize;
    char * buffer;
    size_t result;

    fprintf(stderr,"fopen(%s,\"rb\")\n",verified_filename);
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
      //TODO: make a smaller allocation and gradually serve the whole file :P
      buffer = (char*) malloc (sizeof(char)*lSize);
      if (buffer == 0) { fprintf(stderr," Could not allocate enough memory to serve file %s\n",verified_filename); return 0;}

      // copy the file into the buffer:
      result = fread (buffer,1,lSize,pFile);
      if (result != lSize) {fputs ("Reading error",stderr); return 0;}


      opres=send(clientsock,buffer,result,MSG_WAITALL);  //Send file as soon as we've got it
      /* the whole file is now loaded in the memory buffer. */

      // terminate
      free (buffer);
  }

  fclose (pFile);

  return 1;
  }

 return 0;
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


  // In order for each thread to (in theory) be able to serve a different virtual website
  // we declare the webserver_root etc here and we copy the value from the thread spawning function
  // This creates a little code clutter but it is for the best..!
  char webserver_root[MAX_FILE_PATH]="public_html/";
  char templates_root[MAX_FILE_PATH]="public_html/templates/";

  strncpy(webserver_root,context->webserver_root,MAX_FILE_PATH);
  strncpy(templates_root,context->templates_root,MAX_FILE_PATH);

  int clientsock=context->clientsock;
  int thread_id = context->thread_id;
//  struct sockaddr_in client=context->client;
//  unsigned int clientlen=context->clientlen;
  context->keep_var_on_stack=2;


  struct timeval timeout;
  timeout.tv_sec = (unsigned int) varSocketTimeoutREAD_ms/1000; timeout.tv_usec = 0;
  if (setsockopt (clientsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Receive timeout \n"); }

  timeout.tv_sec = (unsigned int) varSocketTimeoutWRITE_ms/1000; timeout.tv_usec = 0;
  if (setsockopt (clientsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Send timeout \n"); }


  char incoming_request[MAX_HTTP_REQUEST_HEADER]; //A 4K header is more than enough..!

  int close_connection=0;

  while ( (!close_connection)&&(server_running) )
  {
   incoming_request[0]=0;
   int total_header=0,opres=0;
   fprintf(stderr,"Waiting for a valid HTTP header..\n");
   while ( (!HTTPRequestComplete(incoming_request,total_header))&&(server_running) )
   { //Gather Header until http request contains two newlines..!
     opres=recv(clientsock,&incoming_request[total_header],MAX_HTTP_REQUEST_HEADER-total_header,0);
     if (opres<=0)
      {
        //TODO : Check opres here..!
        close_connection=1; // Close_connection controls the receive "keep-alive" loop
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

   if (!result) {  /*We got a bad http request so we will rig it to make server emmit the 400 message*/
                   fprintf(stderr,"Bad Request!");
                   char servefile[MAX_FILE_PATH]={0};
                   SendFile(clientsock,servefile,0,400,0,0,0,templates_root);
                   close_connection=1;
                }
       else
   { // Not a Bad request Start

     if (!output.keepalive) { close_connection=1; } // Close_connection controls the receive "keep-alive" loop

     if ( (output.requestType==GET)||(output.requestType==HEAD))
     {

      char servefile[MAX_FILE_PATH]={0};
      int resource_is_a_directory=0;
      if (strlen(output.resource)>0)
       {
         if (output.resource[strlen(output.resource)-1]=='/')
           {
             resource_is_a_directory=1;
           }
       }

      if ( (strcmp(output.resource,"/")==0) || (resource_is_a_directory) )
        {
          //We have a general request for an index file so we will try to find one , or generate a directory list..!


          int generate_directory_list=0;

          if (resource_is_a_directory)
           {
             /*resource_is_a_directory means we got something like directory1/directory2/ so we should check for index file at the path given..! */
             if ( FindIndexFile(webserver_root,output.resource,servefile) ) { /*servefile should contain a valid index file*/ } else
                {
                 generate_directory_list=1;
                }
           } else
           {
             /*This is the case where we got a / as a request so we give "" as a dir parameter! */
             if ( FindIndexFile(webserver_root,"",servefile) ) { /*servefile should contain a valid index file*/ } else
                {
                 generate_directory_list=1;
                }
           }


         if (generate_directory_list)
             {
               fprintf(stderr,"TODO: Generate index file dynamically here , not implemented..!");
               SendFile(clientsock,servefile,0,501,0,0,0,templates_root);
               close_connection=1;
               //Generate index file dynamically!
               servefile[0]=0; // This prevents standard SendFile from beeing sent!
             }

        } else
       {
         //We have a specific request for a file ( output.resource )
         strcpy(servefile,webserver_root); strcat(servefile,output.resource);
       }



      //SendFile decides about the safety of the resource requested..
      //it should deny requests to paths like ../ or /etc/passwd
      if (servefile[0]!=0) //This means that we have found a file to serve..!
      {
       if ( !SendFile(clientsock,servefile,0,0,(output.requestType==HEAD),output.keepalive,output.supports_gzip,templates_root) )
         {
           //We where unable to serve request , closing connections..\n
           fprintf(stderr,"We where unable to serve request , closing connections..\n");
           close_connection=1;
         }
      }
     } else
     if (output.requestType==NONE)
     {
       fprintf(stderr,"Weird Request!");
       char servefile[MAX_FILE_PATH]={0};
       SendFile(clientsock,servefile,0,400,0,0,0,templates_root);
       close_connection=1;
     } else
     {
       fprintf(stderr,"Not Implemented Request!");
       char servefile[MAX_FILE_PATH]={0};
       SendFile(clientsock,servefile,0,501,0,0,0,templates_root);
       close_connection=1;
     }
   } // Not a Bad request END

  }

  }
  close(clientsock);


  pthread_mutex_lock (&thread_pool_access); // LOCK PROTECTED OPERATION -------------------------------------------
  if (ACTIVE_CLIENT_THREADS>0)
  {
    threads_pool[thread_id]=threads_pool[ACTIVE_CLIENT_THREADS-1];
    threads_pool[ACTIVE_CLIENT_THREADS-1]=0;
    --ACTIVE_CLIENT_THREADS;
  }
  pthread_mutex_unlock (&thread_pool_access); // LOCK PROTECTED OPERATION -------------------------------------------
  pthread_exit(0);
  return 0;
}



int SpawnThreadToServeNewClient(int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root)
{
  fprintf(stderr,"Server Thread : Client connected: %s , %u total active threads\n", inet_ntoa(client.sin_addr),ACTIVE_CLIENT_THREADS);

  if (ACTIVE_CLIENT_THREADS>=MAX_CLIENT_THREADS)
   {
     fprintf(stderr,"We are over the limit on clients served..\nDropping client %s..!\n",inet_ntoa(client.sin_addr));
     close(clientsock);
     return 0;
   }

  struct PassToHTTPThread context;
  memset(&context,0,sizeof(struct PassToHTTPThread));

  context.clientsock=clientsock;
  context.client=client;
  context.clientlen=clientlen;
  strncpy(context.webserver_root,webserver_root,MAX_FILE_PATH);
  strncpy(context.templates_root,templates_root,MAX_FILE_PATH);

  context.keep_var_on_stack=1;

  pthread_mutex_lock (&thread_pool_access); // LOCK PROTECTED OPERATION -------------------------------------------
  context.thread_id = ACTIVE_CLIENT_THREADS++;
  pthread_mutex_unlock (&thread_pool_access); // LOCK PROTECTED OPERATION -------------------------------------------

  int retres = pthread_create(&threads_pool[context.thread_id],0,ServeClient,(void*) &context);
  if ( retres==0 ) { while (context.keep_var_on_stack==1) { usleep(100); } } // <- Keep PeerServerContext in stack for long enough :P


  if (retres!=0) retres = 0; else retres = 1;

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

  char webserver_root[MAX_FILE_PATH]="public_html/";
  char templates_root[MAX_FILE_PATH]="public_html/templates/";

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

  strncpy(webserver_root,context->webserver_root,MAX_FILE_PATH);
  strncpy(templates_root,context->templates_root,MAX_FILE_PATH);

  context->keep_var_on_stack=2;

  if ( bind(serversock,(struct sockaddr *) &server,serverlen) < 0 ) { error("Server Thread : Error binding master port!"); server_running=0; return 0; }
  if ( listen(serversock,MAX_CLIENT_THREADS) < 0 )  { error("Server Thread : Failed to listen on server socket"); server_running=0; return 0; }


  while (stop_server==0)
  {
    printf("Server Thread : Waiting for a new client");
    /* Wait for client connection */
    if ( (clientsock = accept(serversock,(struct sockaddr *) &client, &clientlen)) < 0) { error("Server Thread : Failed to accept client connection"); }
      else
      {
           fprintf(stderr,"Server Thread : Accepted new client \n");
           if (!SpawnThreadToServeNewClient(clientsock,client,clientlen,webserver_root,templates_root))
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



int StartHTTPServer(char * ip,unsigned int port,char * root_path,char * templates_path)
{
  struct PassToHTTPThread context;
  memset(&context,0,sizeof(context));

  strncpy(context.ip,ip,255);
  strncpy(context.webserver_root,root_path,MAX_FILE_PATH);
  strncpy(context.templates_root,templates_path,MAX_FILE_PATH);

  context.port=port;
  context.keep_var_on_stack=1;

  server_running=1;
  pause_server=0;
  stop_server=0;

  int retres=1;

  retres = pthread_create( &server_thread_id ,0,HTTPServerThread,(void*) &context);
  if (retres!=0) retres = 0; else retres = 1;

  while (context.keep_var_on_stack==1) { usleep(100); /*wait;*/ }

  return retres;
}

int StopHTTPServer()
{
  /*
     We want to stop the server that accepts new connections ( and we do that by signaling stop_server=1; )
     The problem is that it will keep waiting for one more job since the server is blocked in an accept call..!
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
