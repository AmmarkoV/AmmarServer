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

#include "tools/directory_lists.h"
#include "server_threads.h"
#include "file_server.h"
#include "cache/client_list.h"
#include "header_analysis/http_header_analysis.h"
#include "tools/http_tools.h"
#include "cache/file_caching.h"
#include "server_configuration.h"

unsigned int GLOBAL_KILL_SERVER_SWITCH = 0;

struct PassToPreSpawnedThread
{
    struct AmmServer_Instance * instance;
    unsigned int i_adapt;
};

struct PassToHTTPThread
{
     struct AmmServer_Instance * instance;

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


void * ServeClient(void * ptr);
void * HTTPServerThread (void * ptr);

int HTTPServerIsRunning(struct AmmServer_Instance * instance)
{
  if (instance==0) { return 0; } //We can't be running not even the instance is allocated..
  return instance->server_running;
}


unsigned int ServerThreads_DropRootUID()
{
   if (ENABLE_DROPPING_UID_ALWAYS ) { /* We fall through and change UID , as a mandatory step.. */} else
   if (ENABLE_DROPPING_ROOT_UID_IF_ROOT) { /*Check if we are root */
                                           if (getuid()>=1000) { /*Non root id , we can skip dropping our UID with this configuration..*/ return 0; }
                                           //If we fell through it means we are root and dropping root when root is enabled so the code that follows will alter our uid..
                                         } else
                                          { fprintf(stderr,"DropRootUID() not needed ..\n"); return 0; }

   char command_to_get_uid[MAX_FILE_PATH]={0};
   sprintf(command_to_get_uid,"id -u %s",USERNAME_UID_FOR_DAEMON);


   FILE * fp  = popen(command_to_get_uid, "r");
   if (fp == 0 ) { fprintf(stderr,"Failed to get our user id ( trying to drop root UID ) \n"); return 0; }

   char output[101]={0};
   /* Read the output a line at a time - output it. */
     unsigned int i=0;
     while (fgets(output,101 , fp) != 0) { ++i; /*fprintf(stderr,"\n\nline %u = %s \n",i,output);*/ break; }
    /* close */
     pclose(fp);

   int non_root_uid = atoi(output);
   if (non_root_uid<1000)
      {
        fprintf(stderr,"The user set in USERNAME_UID_FOR_DAEMON=\"%s\" is also root (his uid is %u)\n",USERNAME_UID_FOR_DAEMON,non_root_uid);
        if (CHANGE_TO_UID<1000) { fprintf(stderr,"Our CHANGE_TO_UID value is also super user , setting a bogus non-root value..\n"); non_root_uid=1500; } else
                                { non_root_uid=CHANGE_TO_UID; }
      }

   fprintf(stderr,"setuid(%u);\n",non_root_uid);
   return setuid(non_root_uid); // Non Root UID :-P
}


int callClientRequestHandler(struct AmmServer_Instance * instance,struct HTTPRequest * output)
{
  if ( instance->clientRequestHandlerOverrideContext == 0 )  { return 0; }
  struct AmmServer_RequestOverride_Context * clientOverride = instance->clientRequestHandlerOverrideContext;
  if ( clientOverride->request_override_callback == 0 ) { return 0; }

  clientOverride->request = output;


  void ( *DoCallback) ( struct AmmServer_RequestOverride_Context * ) = 0 ;
  DoCallback = clientOverride->request_override_callback;

  DoCallback(clientOverride);

  //After getting back the override and whatnot , keep the client from using a potentially bad
  //memory chunk
  clientOverride->request = 0;

  return 1;
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

  struct AmmServer_Instance * instance = context->instance;

  context->keep_var_on_stack=2; //This signals that the thread has processed the message it received..!

  fprintf(stderr,"Passing message to HTTP thread is done \n");
  if (instance==0) { fprintf(stderr,"Serve Client called without a valid instance , it cannot continue \n"); return 0; }
  fprintf(stderr,"ServeClient instance pointing @ %p \n",instance);


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

  while ( (!close_connection) && (instance->server_running) )
  {
   incoming_request[0]=0;
   int total_header=0,opres=0;
   fprintf(stderr,"KeepAlive Server Loop , Waiting for a valid HTTP header..\n");
   while ( (!HTTPRequestComplete(incoming_request,total_header))&&(instance->server_running) )
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


   int result = AnalyzeHTTPRequest(instance,&output,incoming_request,total_header,webserver_root);
   if (result)
      { //If we use a client based request handler , call it now
        callClientRequestHandler(instance,&output);
      }


   if (!result)
   {  /*We got a bad http request so we will rig it to make server emmit the 400 message*/
      fprintf(stderr,"Bad Request!");
      char servefile[MAX_FILE_PATH]={0};
      SendFile(instance,&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
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
   if ((instance->settings.PASSWORD_PROTECTION)&&(!output.authorized))
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


              //TODO ADD Here a possibly rfc1867 , HTTP POST FILE compatible (multipart/form-data) recv handler..
              //TODO TODO TODO


              if (total_header>=MAX_QUERY*4)
              {
                 //Too large request .. We cannot handle it ..
                  char servefile[MAX_FILE_PATH]={0};
                  SendFile(instance,&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
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
      if (ChangeRequestIfInternalRequestIsAddressed(instance,servefile,templates_root) )
      { //Skip disk access times for checking for directories and other stuff..!
        //We know that the resource is a file from our cache indexes..!
          resource_is_a_directory=0;
          resource_is_a_file=1;
      }


      //STEP 0 : Check with cache!
      unsigned int index=0;
      if (CachedVersionExists(instance,servefile,&index) )
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
             if ( FindIndexFile(instance,webserver_root,output.resource,servefile) )
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
          SendFile(instance,&output,clientsock,servefile,0,0,404,0,0,0,templates_root);
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
        SendFile(instance,&output,clientsock,servefile,0,0,404,0,0,0,templates_root);
        close_connection=1;
        we_can_send_result=0;
     }



      //SendFile decides about the safety of the resource requested..
      //it should deny requests to paths like ../ or /etc/passwd
      if (we_can_send_result) //This means that we have found a file to serve..!
      {
       if ( !SendFile (
                        instance,
                        &output,
                        clientsock, // -- Client socket
                        servefile,  // -- Filename to be served
                        output.range_start,  // -- In case of a range request , byte to start
                        output.range_end,    // -- In case of a range request , byte to end
                        0 /*DO NOT FORCE AN ERROR CODE , NORMAL SENDFILE*/ ,
                        (output.requestType==HEAD),
                        output.keepalive,
                        output.supports_compression,
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
       SendFile(instance,&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
       close_connection=1;
     } else
     if (output.requestType==NONE)
     { //We couldnt find a request type so it is a weird input that doesn't seem to be HTTP based
       fprintf(stderr,"Weird unrecognized Request!");
       char servefile[MAX_FILE_PATH]={0};
       SendFile(instance,&output,clientsock,servefile,0,0,400,0,0,0,templates_root);
       close_connection=1;
     } else
     { //The request we got requires not implemented functionality , so we will admit not implementing it..! :P
       fprintf(stderr,"Not Implemented Request!");
       char servefile[MAX_FILE_PATH]={0};
       SendFile(instance,&output,clientsock,servefile,0,0,501,0,0,0,templates_root);
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
     if (instance->threads_pool[thread_id]==0) { fprintf(stderr,"While exiting thread , thread_pool id[%u] is already zero.. This could be a bug ..\n",thread_id); }
     instance->threads_pool[thread_id]=0;
     ++instance->CLIENT_THREADS_STOPPED;

     //We also only want to stop the thread if itsnot prespawned !
     pthread_exit(0);
   }

  return 0;
}

unsigned int FindAProperThreadID(struct AmmServer_Instance * instance,unsigned int starting_from)
{
    if (starting_from>=MAX_CLIENT_THREADS) { starting_from = starting_from % MAX_CLIENT_THREADS; }


   fprintf(stderr,"FindAProperThreadID instance pointing @ %p \n",instance);//Clear instance..!
    fprintf(stderr,"FindAProperThreadID thread pool pointing @ %p \n",instance->threads_pool);//Clear instance..!
    while ( 1 )
     {
       while (starting_from<MAX_CLIENT_THREADS)
         {
            if ( instance->threads_pool[starting_from]==0 ) { return starting_from; }
            ++starting_from;
          }
       starting_from=0;
       fprintf(stderr,"Looped .. while finding a proper thread id..\n");
     }

    return starting_from;
}


int SpawnThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root)
{
  fprintf(stderr,"Server Thread : Client connected: %s , %u total active threads\n", inet_ntoa(client.sin_addr),instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED);

  fprintf(stderr,"SpawnThreadToServeNewClient instance pointing @ %p \n",instance);

  if (instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED >= MAX_CLIENT_THREADS)
   {
     //This needs a little thought.. it is not "nice" to drop clients  , on the other hand what`s the point on opening and
     //maintaining an idle TCP/IP connection when we are on our limits thread-wise..
     fprintf(stderr,"We are over the limit on clients served ( %u threads ) ..\nDropping client %s..!\n",instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED,inet_ntoa(client.sin_addr));
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

  //I Have removed the lock here , so getting a thread_id is a little more complex ..
  context.thread_id = instance->CLIENT_THREADS_STARTED++;
  context.thread_id = FindAProperThreadID(instance,context.thread_id);

  context.instance = instance;

  context.keep_var_on_stack=1;


  fprintf(stderr,"Spawning a new thread %u/%u to serve this client\n",instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED,MAX_CLIENT_THREADS);
  int retres = pthread_create(&instance->threads_pool[context.thread_id],0,ServeClient,(void*) &context);
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
  //We are a thread so lets retrieve our variables..
  struct PassToPreSpawnedThread * incoming_context = (struct PassToPreSpawnedThread *) ptr;

  struct AmmServer_Instance * instance = incoming_context->instance;
  unsigned int i = incoming_context->i_adapt;
  incoming_context->i_adapt = MAX_CLIENT_PRESPAWNED_THREADS+1; // <-- This signals we got the i value..


  if (instance==0) { fprintf(stderr,"Prespawned thread did not receive a valid instance context\n"); return 0; }
  //We will also spawn our own threads so lets prepare their variables..
  struct PassToHTTPThread context; // <-- This is the static copy of the context we will pass through
  memset(&context,0,sizeof(struct PassToHTTPThread)); // We clear it out


  struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
  struct PreSpawnedThread * prespawned_data;
  prespawned_data = (struct PreSpawnedThread *) &prespawned_pool[i];

  while ( (instance->stop_server==0) && (GLOBAL_KILL_SERVER_SWITCH==0) )
   {
      //fprintf(stderr,"Prespawned Thread %u waiting ( its %u's turn ) \n",i,prespawn_turn_to_serve);
          /*It is our turn!!*/
          if (prespawned_data->busy) //Master thread considers us busy again , this means there is work to be done..!
          {
            ++instance->prespawn_jobs_started;
            /*We have something to do , lets fill our context..*/
             context.instance=instance;
             context.clientsock=prespawned_data->clientsock;
             context.client=prespawned_data->client;
             context.clientlen=prespawned_data->clientlen;
             context.pre_spawned_thread = 1; // THIS IS A !!!!PRE SPAWNED!!!! THREAD
             strncpy(context.webserver_root,prespawned_data->webserver_root,MAX_FILE_PATH);
             strncpy(context.templates_root,prespawned_data->templates_root,MAX_FILE_PATH);
             context.keep_var_on_stack=1;

              //ServeClient from this thread ( without forking..! )
              fprintf(stderr,"Prespawned thread %u/%u starting to serve new client\n",i,MAX_CLIENT_PRESPAWNED_THREADS);
                ServeClient((void *)  &context);
              //---------------------------------------------------

             prespawned_data->busy=0; // <- This signals we finished our task ..!
             ++instance->prespawn_jobs_finished;
           }

      if (instance->prespawn_turn_to_serve==i)
            { usleep(THREAD_SLEEP_TIME_WHEN_OUR_PRESPAWNED_THREAD_IS_NEXT); /*It is our turn next so lets stay vigilant ( But not use a crazy lot of CPU time ) */ }  else
               { usleep(THREAD_SLEEP_TIME_FOR_PRESPAWNED_THREADS); /*It is not our turn so lets chill for more time..*/ }
  } // while the server doesn't stop..

  return 0;
}

void PreSpawnThreads(struct AmmServer_Instance * instance)
{
  if (MAX_CLIENT_PRESPAWNED_THREADS==0) { fprintf(stderr,"PreSpawning Threads is disabled , alter MAX_CLIENT_PRESPAWNED_THREADS to enable it..\n"); }

  if ( (instance==0)||(instance->prespawned_pool==0) ) { fprintf(stderr,"PreSpawnThreads called on an invalid instance..\n"); return; }

  struct PassToPreSpawnedThread context;
  memset(&context,0,sizeof(struct PassToPreSpawnedThread));

  struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
  struct PreSpawnedThread * prespawned_data=0;

  unsigned int i=0;
  for (i=0; i<MAX_CLIENT_PRESPAWNED_THREADS; i++)
   {

      prespawned_data = (struct PreSpawnedThread *) &prespawned_pool[i];

      context.instance = instance;
      context.i_adapt = i;
      prespawned_data->busy=0; // We do this here (and not in the PreSpawnedThread ) to make sure a clean state is sure to be initialized , not having race conditions , locks etc...
      int retres = pthread_create(&prespawned_data->thread_id,0,PreSpawnedThread,(void*) &context );
      if ( retres==0 ) { while (context.i_adapt==i) { usleep(1); } } // <- Keep i value the same for long enough without locks
   }
}


int UsePreSpawnedThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root)
{
   //Please note that this must only get called from the main process/thread..
   fprintf(stderr,"UsePreSpawnedThreadToServeNewClient instance pointing @ %p \n",instance);

   struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
   struct PreSpawnedThread * prespawned_data=0;

   if (MAX_CLIENT_PRESPAWNED_THREADS==0) { fprintf(stderr,"PreSpawning Threads is disabled , alter MAX_CLIENT_PRESPAWNED_THREADS to enable it..\n"); }
   if (instance->prespawn_jobs_started<instance->prespawn_jobs_finished ) {  fprintf(stderr,"Prespawn jobs counters truncated (?) \n"); } else
   if (instance->prespawn_jobs_started-instance->prespawn_jobs_finished<MAX_CLIENT_PRESPAWNED_THREADS)
    {
        prespawned_data = &prespawned_pool[instance->prespawn_turn_to_serve];

        if (!prespawned_data->busy)
         {
             fprintf(stderr,"Decided to use prespawned thread %u/%u to serve new client\n",instance->prespawn_turn_to_serve,MAX_CLIENT_PRESPAWNED_THREADS);
             prespawned_data->clientsock=clientsock;
             prespawned_data->client=client;
             prespawned_data->clientlen=clientlen;
             strncpy(prespawned_data->webserver_root,webserver_root,MAX_FILE_PATH);
             strncpy(prespawned_data->templates_root,templates_root,MAX_FILE_PATH);
             // The busy byte gets filled in last because it is what causes the client thread to wake up..!
             prespawned_data->busy=1;

             ++instance->prespawn_turn_to_serve;
             instance->prespawn_turn_to_serve = instance->prespawn_turn_to_serve % MAX_CLIENT_PRESPAWNED_THREADS; // <- Round robin next thread..

             return 1;
         } else
         {
            fprintf(stderr,"Seems that the prespaned thread %u/%u is still busy ..\n",instance->prespawn_turn_to_serve,MAX_CLIENT_PRESPAWNED_THREADS);
            return 0;
         }
    } else
    {
        fprintf(stderr,"All prespawned threads are busy.. ( start %u , end %u , max %u) \n",instance->prespawn_jobs_started,instance->prespawn_jobs_finished,MAX_CLIENT_PRESPAWNED_THREADS);
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
  if (context==0) { fprintf(stderr,"Error , HTTPServerThread called without a context\n"); /*We dont have a context , so we cant signal anything :P */ return 0; }


  unsigned int serverlen = sizeof(struct sockaddr_in),clientlen = sizeof(struct sockaddr_in);
  struct sockaddr_in server;
  struct sockaddr_in client;

  struct AmmServer_Instance * instance = context->instance;
  if (instance==0) { fprintf(stderr,"Error , HTTPServerThread called with an invalid instance\n"); context->keep_var_on_stack=2;  return 0; }
  fprintf(stderr,"HTTPServerThread instance pointing @ %p \n",instance);

  int serversock = socket(AF_INET, SOCK_STREAM, 0);
    if ( serversock < 0 ) { error("Server Thread : Opening socket"); instance->server_running=0; context->keep_var_on_stack=2;  return 0; }
  instance->serversock = serversock;

  bzero(&client,clientlen);
  bzero(&server,serverlen);

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(context->port);

  strncpy(webserver_root,context->webserver_root,MAX_FILE_PATH);
  strncpy(templates_root,context->templates_root,MAX_FILE_PATH);


  //We bind to our port..!
  if ( bind(serversock,(struct sockaddr *) &server,serverlen) < 0 )
    {
      error("Server Thread : Error binding master port!\nThe server may already be running ..\n");
      instance->server_running=0;
      context->keep_var_on_stack=2; //If we were not able to bind we still signal that we got the message so that parent thread can continue
      return 0;
    }

  //If we managed to bind we return success! so that the parent thread can continue with its work..
  context->keep_var_on_stack=2;

  //MAX_CLIENT_THREADS <- this could also be used instead of MAX_CLIENTS_LISTENING_FOR
  //I am trying a larger listen queue to hold incoming connections regardless of the serving threads
  //so that they will be used later
  if ( listen(serversock,MAX_CLIENTS_LISTENING_FOR /*MAX_CLIENT_THREADS*/) < 0 )  //Note that we are listening for a max number of clients as big as our maximum thread number..!
           { error("Server Thread : Failed to listen on server socket"); instance->server_running=0; return 0; }



  //If we made it this far , it means we got ourselves the port we wanted and we can start serving requests , but before we do that..
  //The next call Pre"forks" a number of threads specified in configuration.h ( MAX_CLIENT_PRESPAWNED_THREADS )
  //They can reduce latency by up tp 10ms on a Raspberry Pi , without any side effects..
  PreSpawnThreads(instance);

  while ( (instance->stop_server==0) && (GLOBAL_KILL_SERVER_SWITCH==0) )
  {
    fprintf(stderr,"\nServer Thread : Waiting for a new client\n");
    /* Wait for client connection */
    int clientsock=0;
    if ( (clientsock = accept(serversock,(struct sockaddr *) &client, &clientlen)) < 0) { error("Server Thread : Failed to accept client connection"); }
      else
      {
           fprintf(stderr,"Server Thread : Accepted new client , now deciding on prespawned vs freshly spawned.. \n");

           if (UsePreSpawnedThreadToServeNewClient(instance,clientsock,client,clientlen,webserver_root,templates_root))
            {
              // This request got served by a prespawned thread..!
              // Nothing to do here , proceeding to the next incoming connection..
              // if we failed to use a pre spawned thread we will spawn a new one using the next call..!
            } else
           if (!SpawnThreadToServeNewClient(instance,clientsock,client,clientlen,webserver_root,templates_root))
            {
                fprintf(stderr,"Server Thread : Client failed, while handling him\n");
                close(clientsock);
            }
      }
 }
  instance->server_running=0;
  instance->stop_server=2;

  //It should already be closed so skipping this : close(serversock);
  pthread_exit(0);
  return 0;
}




int StartHTTPServer(struct AmmServer_Instance * instance,char * ip,unsigned int port,char * root_path,char * templates_path)
{
  if (instance==0) { fprintf(stderr,"StartHTTPServer called with an unallocated instance \n"); return 0; }

  //Since this webserver is "serious-stuff" we may want to increase its priority..
   if ( CHANGE_PRIORITY != 0 )
    { if ( nice(CHANGE_PRIORITY) == -1 ) { fprintf(stderr,"Error changing process priority to %i \n",CHANGE_PRIORITY); } else
                                         { fprintf(stderr,"Changed priority to %i \n",CHANGE_PRIORITY); } }
  //-------------------------------------------------------------------------------------------------------------




  struct PassToHTTPThread context;
  memset(&context,0,sizeof(context));

  strncpy(context.ip,ip,255);
  strncpy(context.webserver_root,root_path,MAX_FILE_PATH);
  strncpy(context.templates_root,templates_path,MAX_FILE_PATH);

  context.port=port;
  context.instance = instance; //Also pass instance on new thread..
  context.keep_var_on_stack=1;

  instance->server_running=1;
  instance->pause_server=0;
  instance->stop_server=0;


  fprintf(stderr,"StartHTTPServer instance pointing @ %p \n",instance);


  //Creating the main WebServer thread..
  //It will bind the ports and start receiving requests and pass them over to new and prespawned threads
  pthread_t server_thread_id;
  int retres = pthread_create( &server_thread_id ,0,HTTPServerThread,(void*) &context);
  instance->server_thread_id = server_thread_id;
  //If pthread_creation was a success, we wait for the new thread to get its configuration parameters..
  if ( retres==0 ) { while (context.keep_var_on_stack==1) { usleep(1); /*wait;*/ } }
  //We flip the retres
  if (retres!=0) retres = 0; else retres = 1;



   //After changing our priority , binding our port ( which could be 80 and could require superuser powers ) , it may be time to drop RootUID
   //There could be a user like www-run or something else that we could setuid to , but this isn't yet implemented...
   ServerThreads_DropRootUID();

/*
  //The next call simulates an incoming request that gets served by the server in order to test it and preload the index page for better performance..!
  char * file = RequestHTTPWebPage("127.0.0.1",port,"\0",100);
  if (file!=0) { free(file); fprintf(stderr,"Internal Index Request was succesful..\n"); } else
               { fprintf(stderr,"Internal Index Request failed ..\n"); } */

  return retres;
}

int StopHTTPServer(struct AmmServer_Instance * instance)
{
  /*
     We want to stop the server that accepts new connections ( and we do that by signaling instance->stop_server=1; )
     The problem is that it will keep waiting for one more job since the server is blocked in an accept call..!
     Thats why we force the socket close which in turn terminates the server thread..
  */
  if ( (instance->stop_server==2)||(instance->stop_server==0)) { fprintf(stderr,"Server has stopped working on its own..\n"); return 1;}
  fprintf(stderr,"Force closing bind socket... ");
  close(instance->serversock);

  instance->stop_server=1;
  fprintf(stderr,"Waiting for Server to stop.. ");

  while (instance->stop_server!=2)
    {
        fprintf(stderr,".");
        usleep(10000);
    }
  fprintf(stderr," .. done \n");

  return (instance->stop_server==2);
}


