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

#include "directory_lists.h"
#include "server_threads.h"
#include "file_server.h"
#include "client_list.h"
#include "http_header_analysis.h"
#include "http_tools.h"
#include "file_caching.h"
#include "configuration.h"


int serversock;
int server_running=0;
int pause_server=0;
int stop_server=0;
pthread_t server_thread_id;

pthread_mutex_t thread_pool_access;
int CLIENT_THREADS_STARTED=0;
int CLIENT_THREADS_STOPPED=0;
pthread_t threads_pool[MAX_CLIENT_THREADS]={0};

struct PreSpawnedThread
{
    pthread_t thread_id;

    int clientsock;
    struct sockaddr_in client;
    unsigned int clientlen;

    char webserver_root[MAX_FILE_PATH];
    char templates_root[MAX_FILE_PATH];
};


struct PassToHTTPThread
{
     char ip[256];
     char webserver_root[MAX_FILE_PATH];
     char templates_root[MAX_FILE_PATH];

     unsigned int port;
     unsigned int keep_var_on_stack;
     int pre_spawned_thread;

     int clientsock;
     struct sockaddr_in client;
     unsigned int clientlen;
     unsigned int thread_id;
};

unsigned int prespawn_turn_to_serve=0,prespawn_jobs_started=0,prespawn_jobs_finished=0;
struct PreSpawnedThread prespawned_pool[MAX_CLIENT_PRESPAWNED_THREADS]={0};

void * ServeClient(void * ptr);
void * HTTPServerThread (void * ptr);

int HTTPServerIsRunning()
{
  return server_running;
}

/*
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
  //! One thing to remember is that we shouldnt return anywhere BUT the end of this function to keep
  //! correct tracking of active threads , when something bad happens and we want to return , close_connection=1; is set

  fprintf(stderr,"New Serve Client call ..\n");
  struct PassToHTTPThread * context = (struct PassToHTTPThread *) ptr;


  // In order for each thread to (in theory) be able to serve a different virtual website
  // we declare the webserver_root etc here and we copy the value from the thread spawning function
  // This creates a little code clutter but it is for the best..!
  char webserver_root[MAX_FILE_PATH]="public_html/";
  char templates_root[MAX_FILE_PATH]="public_html/templates/";


  strncpy(webserver_root,context->webserver_root,MAX_FILE_PATH);
  strncpy(templates_root,context->templates_root,MAX_FILE_PATH);

//  char * spn = strstr (templates_root,webserver_root);
//  if (spn==0) { /*templates_root is not the same path*/ }

  int pre_spawned_thread=context->pre_spawned_thread;
  int clientsock=context->clientsock;
  int thread_id = context->thread_id;

  context->keep_var_on_stack=2; //This signals that the thread has processed the message it received..!
  fprintf(stderr,"Passing message to HTTP thread is done \n");


  struct timeval timeout;
  timeout.tv_sec = (unsigned int) varSocketTimeoutREAD_ms/1000; timeout.tv_usec = 0;
  if (setsockopt (clientsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Receive timeout \n"); }

  timeout.tv_sec = (unsigned int) varSocketTimeoutWRITE_ms/1000; timeout.tv_usec = 0;
  if (setsockopt (clientsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Send timeout \n"); }




   //Now the real fun starts :P <- helpfull comment
   unsigned int client_id=GetClientId("0.0.0.0"); // <- TODO add IPv4 , IPv6 IP here
   if ( ClientIsBanned(client_id) )
     {
         SendErrorCodeHeader(clientsock,403 /*Forbidden*/,"403.html",templates_root);
     } else

  { /*!START OF CLIENT IS NOT ON IP-BANNED-LIST!*/

  char incoming_request[MAX_HTTP_REQUEST_HEADER+1]; //A 4K header is more than enough..!

  int close_connection=0;

  while ( (!close_connection)&&(server_running) )
  {
   incoming_request[0]=0;
   int total_header=0,opres=0;
   fprintf(stderr,"KeepAlive Server Loop , Waiting for a valid HTTP header..\n");
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
       if (total_header>=MAX_HTTP_REQUEST_HEADER)
        {
           fprintf(stderr,"The request would overflow , dropping client \n");
           opres=0;
           close_connection=1;
        }
      }
   }


  if (opres>0)
  {
   fprintf(stderr,"Received request header \n");
   //fprintf(stderr,"Received %s \n",incoming_request);
   struct HTTPRequest output; // This should get free'ed once it isn't needed any more see FreeHTTPRequest call!
   memset(&output,0,sizeof(struct HTTPRequest));


   int result = AnalyzeHTTPRequest(&output,incoming_request,total_header,webserver_root);

   if (!result)
   {  /*We got a bad http request so we will rig it to make server emmit the 400 message*/
      fprintf(stderr,"Bad Request!");
      char servefile[MAX_FILE_PATH]={0};
      SendFile(&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
      close_connection=1;
   }
      else
   if (!AllowClientToUseResource(client_id,output.resource))
   {
     //Client is forbidden but he is not IP banned to use resource ( already opened too many connections or w/e other reason )
     //Doesnt have access to the specific file , etc..!
     SendErrorCodeHeader(clientsock,403 /*Forbidden*/,"403.html",templates_root);
     close_connection=1;
   } else
   if ((PASSWORD_PROTECTION)&&(!output.authorized))
   {
     SendAuthorizationHeader(clientsock,"AmmarServer authorization..!","authorization.html");

     char reply_header[256]={0};
     strcpy(reply_header,"\n\n<html><head><title>Authorization needed</title></head><body><br><h1>Unauthorized access</h1><h3>Please note that all unauthorized access attempts are logged ");
     strcat(reply_header,"and your host machine will be permenantly banned if you exceed the maximum number of incorrect login attempts..</h2></body></html>\n");
     int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send file as soon as we've got it

     if (opres<=0) { fprintf(stderr,"Error sending authorization needed message\n"); }
     close_connection=1;
   }
     else
   { // Not a Bad request Start

     if (!output.keepalive) { close_connection=1; } // Close_connection controls the receive "keep-alive" loop


      if ( ( output.requestType==POST ) && (ENABLE_POST) )
       {

              fprintf(stderr,"POST HEADER : \n %s \n",incoming_request);

              if (total_header>=MAX_QUERY*4)
              {
                 //Too large request .. We cannot handle it ..
                  char servefile[MAX_FILE_PATH]={0};
                  SendFile(&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
                  fprintf(stderr,"Huge POST request ( header size %u , MAX_QUERY size %u )  , drowning it..\n",total_header,MAX_QUERY);
                  close_connection=1;
              } else
              {
                  strncpy(output.POSTquery,incoming_request,MAX_QUERY*4);
                  fprintf(stderr,"Found a POST query , %s \n",output.POSTquery);
                  fprintf(stderr,"Will now pretend that we are a GET request for the rest of the page to be served nicely\n");

                  //Will now pretend that we are a GET request for the rest of the page to be served nicely
                  output.requestType=GET;
              }
       }


     if ( (output.requestType==GET)||(output.requestType==HEAD))
     {
      char servefile[(MAX_FILE_PATH*2)+1]={0}; // Since we are strcat-ing the file on top of the webserver_root it is only logical to
      // reserve space for two MAX_FILE_PATHS they are a software security limitation ( system max_path is much larger ) so its not a problem anywhere..!
      int resource_is_a_directory=0,resource_is_a_file=0,generate_directory_list=0,we_can_send_result=1;

      /*!
             PART 1 : Sense what we want to serve , and set the flags
             resource_is_a_directory , resource_is_a_file , generate_directory_list
             accordingly..!
      */

      // STEP 0 : Is the resource an obvious directory , or obvious file ? Check for it..!
      if  (strcmp(output.resource,"/")==0) { resource_is_a_directory=1; }
        else
      if (strlen(output.resource)>0)
       {
         if (output.resource[strlen(output.resource)-1]=='/')
           {
             resource_is_a_directory=1;
           }
       }

      strncpy(servefile,output.verified_local_resource,MAX_FILE_PATH);
      //servefile variable now contains just the verified_local_resource as generated by http_header_analysis.c
      //we have checked output.verified_local_resource for .. , weird ascii characters etc, so it should be safe for usage from now on..!

      //There are some virtual files we want to re-route to their real path..!
      if (ChangeRequestIfInternalRequestIsAddressed(servefile,templates_root) )
      { //Skip disk access times for checking for directories and other stuff..!
        //We know that the resource is a file from our cache indexes..!
          resource_is_a_directory=0;
          resource_is_a_file=1;
      }


      //STEP 0 : Check with cache!
      unsigned int index=0;
      if (CachedVersionExists(servefile,&index) )
      { //Skip disk access times for checking for directories and other stuff..!
        //We know that the resource is a file from our cache indexes..!
        //Bonus points we now have the index id of the cached instance of the file for future use..
          resource_is_a_directory=0;
          resource_is_a_file=1;
      } else
      {//Start of file not in cache scenario
      // STEP 1 : If we can't obviously determine if the request is a directory ,  lets check on disk to find out if it is a directory after all..!
      if (!resource_is_a_directory)
       {
         if (DirectoryExistsAmmServ(servefile))
         {
           resource_is_a_directory=1;

           //If the resource had an  / in the string end we would have already come to the conclusion that this was a directory
           // so lets append the slash now on the request to help the code that follows work as intended
           strcat(output.resource,"/");
           strcat(servefile,"/");
           fprintf(stderr,"TODO: this path is a little problematic because the web browser on the client will think\n");
           fprintf(stderr,"that we are on the / directory , a location field in the response header will clarify things\n");
         }
       }

      // STEP 2 : If we are sure that we dont have a directory then we have to find out accessing disk , could it be that our client wants a file ?
      if (!resource_is_a_directory)
       {
         if (FileExistsAmmServ(servefile))
         {
           resource_is_a_file=1;
         }
       }




      // STEP 3 : If after these steps the resource turned out to be a directory , we cant serve raw directories , so we will either look for an index.html
      // and if an index file cannot be found we will generate a directory list and send that instead..!
      if (resource_is_a_directory)
           {
             /*resource_is_a_directory means we got something like directory1/directory2/ so we should check for index file at the path given..! */
             if ( FindIndexFile(webserver_root,output.resource,servefile) )
                {
                   /*servefile should contain a valid index file ,
                     lets make it look like it was a file we wanted all along :P! */
                   resource_is_a_directory=0;
                   resource_is_a_file=1;
                } else
                {
                 generate_directory_list=1;
                }
           }
      } //End of file not in cache scenario

      /*!
             PART 2 : The flags
             resource_is_a_directory , resource_is_a_file , generate_directory_list
             have been set to the correct ( :P ) value so all we have to do now is serve the correct repsonse..!
      */
     if (generate_directory_list)
     {
       // We need to generate and serve a directory listing..!

       strncpy(servefile,webserver_root,MAX_FILE_PATH);
       strncat(servefile,output.resource,MAX_FILE_PATH);
       ReducePathSlashes_Inplace(servefile);

       char reply_body[MAX_DIRECTORY_LIST_RESPONSE_BODY+1]={0};
       unsigned long sendSize = GenerateDirectoryPage(servefile,output.resource,reply_body,MAX_DIRECTORY_LIST_RESPONSE_BODY);
       if (sendSize>0)
        {
          //If Directory_listing enabled and directory is ok , send the generated site
          SendMemoryBlockAsFile(clientsock,reply_body,sendSize);
        } else
        {
          //If Directory listing disabled or directory is not ok send a 404
          SendFile(&output,clientsock,servefile,0,0,404,0,0,0,templates_root);
        }
       close_connection=1;
       we_can_send_result=0;
     }
        else
      if  (resource_is_a_file)
      {
         //We have a specific request for a file ( output.resource )
         fprintf(stderr,"It is a file request for %s ..!\n",servefile);
         we_can_send_result=1;
      }
       else
     {
        fprintf(stderr,"404 not found..!!");
        char servefile[MAX_FILE_PATH]={0};
        SendFile(&output,clientsock,servefile,0,0,404,0,0,0,templates_root);
        close_connection=1;
        we_can_send_result=0;
     }



      //SendFile decides about the safety of the resource requested..
      //it should deny requests to paths like ../ or /etc/passwd
      if (we_can_send_result) //This means that we have found a file to serve..!
      {
       if ( !SendFile (
                        &output,
                        clientsock, // -- Client socket
                        servefile,  // -- Filename to be served
                        output.range_start,  // -- In case of a range request , byte to start
                        output.range_end,    // -- In case of a range request , byte to end
                        0 /*DO NOT FORCE AN ERROR CODE , NORMAL SENDFILE*/ ,
                        (output.requestType==HEAD),
                        output.keepalive,
                        output.supports_gzip,
                        templates_root)
                      )
         {
           //We where unable to serve request , closing connections..\n
           fprintf(stderr,"We where unable to serve request , closing connections..\n");
           close_connection=1;
         }
      }
     } else
     //It is not a valid GET / HEAD / POST request , so use the default handlers for Bad / Not Implemented requests..!

     if (output.requestType==BAD)
     { //In case some of the security features of the server sensed a BAD! request we should log it..
       fprintf(stderr,"BAD predatory Request sensed by header analysis!");
       //TODO : call -> int ErrorLogAppend(char * IP,char * DateStr,char * Request,unsigned int ResponseCode,unsigned long ResponseLength,char * Location,char * Useragent)
       char servefile[MAX_FILE_PATH]={0};
       SendFile(&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
       close_connection=1;
     } else
     if (output.requestType==NONE)
     { //We couldnt find a request type so it is a weird input that doesn't seem to be HTTP based
       fprintf(stderr,"Weird unrecognized Request!");
       char servefile[MAX_FILE_PATH]={0};
       SendFile(&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
       close_connection=1;
     } else
     { //The request we got requires not implemented functionality , so we will admit not implementing it..! :P
       fprintf(stderr,"Not Implemented Request!");
       char servefile[MAX_FILE_PATH]={0};
       SendFile(&output,clientsock,servefile,0,0,501,0,0,0,templates_root);
       close_connection=1;
     }
   } // Not a Bad request END


    ClientStoppedUsingResource(client_id,output.resource); // This in order for client_list to correctly track client behaviour..!
    if (!FreeHTTPRequest(&output)) { fprintf(stderr,"WARNING: Could not Free HTTP request , please check FIELDS_TO_CLEAR_FROM_HTTP_REQUEST (%u).. \n",FIELDS_TO_CLEAR_FROM_HTTP_REQUEST); }
  }

  } // Keep-Alive loop  ( not closing socket )

  } /*!END OF CLIENT NOT IP-BANNED CODE !*/

  close(clientsock);

  if (!pre_spawned_thread)
   { //If we are running in a prespawned thread it is wrong to count this thread as a *dynamic* one that just stopped !
     //Clear thread id handler and we can gracefully exit..! ( LOCKLESS OPERATION)
     if (threads_pool[thread_id]==0) { fprintf(stderr,"While exiting thread , thread_pool id[%u] is already zero.. This could be a bug ..\n",thread_id); }
     threads_pool[thread_id]=0;
     ++CLIENT_THREADS_STOPPED;
   }


  pthread_exit(0);
  return 0;
}

unsigned int FindAProperThreadID(unsigned int starting_from)
{
    if (starting_from>=MAX_CLIENT_THREADS) { starting_from = starting_from % MAX_CLIENT_THREADS; }

    while ( 1 )
     {
       while (starting_from<MAX_CLIENT_THREADS) { if ( threads_pool[starting_from]==0 ) { return starting_from; } ++starting_from; }
       starting_from=0;
       fprintf(stderr,"Looped .. while finding a proper thread id..\n");
     }

    return starting_from;
}


int SpawnThreadToServeNewClient(int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root)
{
  fprintf(stderr,"Server Thread : Client connected: %s , %u total active threads\n", inet_ntoa(client.sin_addr),CLIENT_THREADS_STARTED - CLIENT_THREADS_STOPPED);

  if (CLIENT_THREADS_STARTED - CLIENT_THREADS_STOPPED >= MAX_CLIENT_THREADS)
   {
     //This needs a little thought.. it is not "nice" to drop clients  , on the other hand what`s the point on opening and
     //maintaining an idle TCP/IP connection when we are on our limits thread-wise..
     fprintf(stderr,"We are over the limit on clients served ( %u threads ) ..\nDropping client %s..!\n",CLIENT_THREADS_STARTED - CLIENT_THREADS_STOPPED,inet_ntoa(client.sin_addr));
     close(clientsock);
     return 0;
   }

  //TODO : Here would be a good spot to query the client socket ip address in the client_list ..
  //We may want to keep a client for opening too many connections or ban him early on , before going through the expense
  //of creating a seperate thread for him..

  struct PassToHTTPThread context;
  memset(&context,0,sizeof(struct PassToHTTPThread));

  context.clientsock=clientsock;
  context.client=client;
  context.clientlen=clientlen;
  context.pre_spawned_thread = 0; // THIS IS A !!!NEW!!! THREAD , NOT A PRESPAWNED ONE
  strncpy(context.webserver_root,webserver_root,MAX_FILE_PATH);
  strncpy(context.templates_root,templates_root,MAX_FILE_PATH);

  context.keep_var_on_stack=1;

  //I Have removed the lock here , so getting a thread_id is a little more complex ..
  context.thread_id = CLIENT_THREADS_STARTED++;
  context.thread_id = FindAProperThreadID(context.thread_id);

  int retres = pthread_create(&threads_pool[context.thread_id],0,ServeClient,(void*) &context);
  usleep(2); //<- Give some time to the thread to startup
  if ( retres==0 ) { while (context.keep_var_on_stack==1) { /*usleep(1);*/ } } else // <- Keep PeerServerContext in stack for long enough :P
                   { fprintf(stderr,"Could not create a new thread..\n "); }

  if (retres!=0) { retres = 0; } else { retres = 1; }

  return retres;
}




/*

  -----------------------------------------------------------------
  -----------------------------------------------------------------
          /\                                             /\
          ||   NEW THREAD GENERATION AND SERVING         ||
          ||                                             ||
  -----------------------------------------------------------------
  -----------------------------------------------------------------
  -----------------------------------------------------------------
  -----------------------------------------------------------------
          ||                                             ||
          ||  PRE SPAWNED THREADS SERVING CLIENTS        ||
          \/                                             \/
  -----------------------------------------------------------------
  -----------------------------------------------------------------

*/


void * PreSpawnedThread(void * ptr)
{
  int * i_adapt = (int *) ptr;
  int i = *i_adapt;
  *i_adapt = MAX_CLIENT_PRESPAWNED_THREADS+1; // <-- This signals we got the i value..


  struct PassToHTTPThread context; // <-- This is the static copy of the context we will pass through
  memset(&context,0,sizeof(struct PassToHTTPThread)); // We clear it out


  while (stop_server==0)
   {
      if (prespawn_turn_to_serve==i)
       {
          /*It is our turn!!*/
          if (prespawned_pool[i].clientsock!=0)
          {
            ++prespawn_jobs_started;
            /*We have something to do , lets fill our context..*/
             context.clientsock=prespawned_pool[i].clientsock;
             context.client=prespawned_pool[i].client;
             context.clientlen=prespawned_pool[i].clientlen;
             context.pre_spawned_thread = 1; // THIS IS A !!!!PRE SPAWNED!!!! THREAD
             strncpy(context.webserver_root,prespawned_pool[i].webserver_root,MAX_FILE_PATH);
             strncpy(context.templates_root,prespawned_pool[i].templates_root,MAX_FILE_PATH);
             context.keep_var_on_stack=1;

              //ServeClient from this thread ( without forking..! )
              fprintf(stderr,"Prespawned thread %u/%u starting to serve new client\n",i,MAX_CLIENT_PRESPAWNED_THREADS);
                ServeClient((void *)  &context);
              //---------------------------------------------------

             ++prespawn_jobs_finished;
             prespawned_pool[i].clientsock=0; // <- This signals we finished our task ..!
          }  else { usleep(1); /*It is our turn so lets stay vigilant*/ }
       } else { usleep(100); /*It is not our turn so lets chill..*/ }
   }

  return 0;
}

void PreSpawnThreads()
{
  if (MAX_CLIENT_PRESPAWNED_THREADS==0) { fprintf(stderr,"PreSpawning Threads is disabled , alter MAX_CLIENT_PRESPAWNED_THREADS to enable it..\n"); }

  int i=0,thread_i=0;
  for (i=0; i<MAX_CLIENT_PRESPAWNED_THREADS; i++)
   {
      thread_i=i;
      int retres = pthread_create(&prespawned_pool[i].thread_id,0,PreSpawnedThread,(void*) &thread_i );
      if ( retres==0 ) { while (thread_i==i) { usleep(1); } } // <- Keep i value the same for long enough without locks
   }
}


int UsePreSpawnedThreadToServeNewClient(int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root)
{
   if (prespawn_jobs_started-prespawn_jobs_finished<MAX_CLIENT_PRESPAWNED_THREADS)
    {
        ++prespawn_turn_to_serve;
        prespawn_turn_to_serve=prespawn_turn_to_serve%MAX_CLIENT_PRESPAWNED_THREADS; // <- Round robin next thread..
        if (prespawned_pool[prespawn_turn_to_serve].clientsock==0)
         {
             fprintf(stderr,"Decided to use prespawned thread %u/%u to serve new client\n",prespawn_turn_to_serve,MAX_CLIENT_PRESPAWNED_THREADS);
             prespawned_pool[prespawn_turn_to_serve].clientsock=clientsock;
             prespawned_pool[prespawn_turn_to_serve].client=client;
             prespawned_pool[prespawn_turn_to_serve].clientlen=clientlen;
             strncpy(prespawned_pool[prespawn_turn_to_serve].webserver_root,webserver_root,MAX_FILE_PATH);
             strncpy(prespawned_pool[prespawn_turn_to_serve].templates_root,templates_root,MAX_FILE_PATH);

             return 1;
         } else
         {
            //Seems that this thread is not free yet..!
            return 0;
         }
    }
  return 0;
}
/*

  -----------------------------------------------------------------
  -----------------------------------------------------------------
          /\                                             /\
          ||   PRE SPAWNED THREADS SERVING CLIENTS       ||
          ||                                             ||
  -----------------------------------------------------------------
  -----------------------------------------------------------------
  -----------------------------------------------------------------

*/















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

  if ( bind(serversock,(struct sockaddr *) &server,serverlen) < 0 ) { error("Server Thread : Error binding master port!\nThe server may already be running ..\n"); server_running=0; return 0; }
  if ( listen(serversock,MAX_CLIENT_THREADS) < 0 )  //Note that we are listening for a max number of clients as big as our maximum thread number..!
           { error("Server Thread : Failed to listen on server socket"); server_running=0; return 0; }


  while (stop_server==0)
  {
    fprintf(stderr,"\nServer Thread : Waiting for a new client");
    /* Wait for client connection */
    if ( (clientsock = accept(serversock,(struct sockaddr *) &client, &clientlen)) < 0) { error("Server Thread : Failed to accept client connection"); }
      else
      {
           fprintf(stderr,"Server Thread : Accepted new client \n");


           if (UsePreSpawnedThreadToServeNewClient(clientsock,client,clientlen,webserver_root,templates_root))
            {
              // This request got served by a prespawned thread..!
              // Nothing to do here , proceeding to the next incoming connection..
              // if we failed to use a pre spawned thread we will spawn a new one using the next call..!
            } else
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

  while (context.keep_var_on_stack==1) { usleep(1); /*wait;*/ }


  PreSpawnThreads();


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


