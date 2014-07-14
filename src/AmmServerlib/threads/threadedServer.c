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

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>

#include <unistd.h>
#include <pthread.h>

#include "threadedServer.h"

#include "../tools/directory_lists.h"
#include "../network/file_server.h"
#include "../header_analysis/http_header_analysis.h"
#include "../tools/http_tools.h"
#include "../tools/logs.h"
#include "../cache/file_caching.h"
#include "../server_configuration.h"

#include "../threads/freshThreads.h"
#include "../threads/prespawnedThreads.h"
#include "../threads/threadInitHelper.h"

#include "../cache/client_list.h"
#include "../cache/dynamic_requests.h"

int HTTPServerIsRunning(struct AmmServer_Instance * instance)
{
  if (instance==0) { return 0; } //We can't be running not even the instance is allocated..
  return instance->server_running;
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


inline int ServeClientKeepAliveLoop(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
   if ( transaction->incomingHeader.headerRAW!=0 ) { free(transaction->incomingHeader.headerRAW); transaction->incomingHeader.headerRAW=0; }

   unsigned long headerRAWSizeTemp = 0;
   transaction->incomingHeader.headerRAW = ReceiveHTTPHeader(instance,transaction->clientSock,&headerRAWSizeTemp);
   transaction->incomingHeader.headerRAWSize = (unsigned int) headerRAWSizeTemp;
   #warning "Work around with no reason here ?"
   if (transaction->incomingHeader.headerRAW==0) { return 0; }

   fprintf(stderr,"Received request header \n");
   transaction->incomingHeader.POSTrequest=0;
   transaction->incomingHeader.POSTrequestSize=0;

   int result = AnalyzeHTTPHeader(instance,transaction);
   if (result)
      {
        if ( (transaction->incomingHeader.requestType==POST) && (ENABLE_POST) )
        {
           //If we have a POST request
           //Expand header to also receive the files uploaded
           AppendPOSTRequestToHTTPHeader(transaction);
        }
        //If we use a client based request handler , call it now
        callClientRequestHandler(instance,&transaction->incomingHeader);
      }

   if (!result)
   {  /*We got a bad http request so we will rig it to make server emmit the 400 message*/
      error("Bad Request!");
      SendErrorFile(instance,transaction,400);
      return 0;
   }
      else
   if (!clientList_isClientAllowedToUseResource(instance->clientList,transaction->clientListID,transaction->incomingHeader.resource))
   {
     //Client is forbidden but he is not IP banned to use resource ( already opened too many connections or w/e other reason )
     //Doesnt have access to the specific file , etc..!
     warning("Client Denied access to resource!");
     SendErrorCodeHeader(transaction->clientSock,403 /*Forbidden*/,"403.html",instance->templates_root);
     return 0;
   } else
   if ((instance->settings.PASSWORD_PROTECTION)&&(!transaction->incomingHeader.authorized))
   {
     SendAuthorizationHeader(transaction->clientSock,"AmmarServer authorization..!","authorization.html");

     char reply_header[256]={0};
     strcpy(reply_header,"\n\n<html><head><title>Authorization needed</title></head><body><br><h1>Unauthorized access</h1><h3>Please note that all unauthorized access attempts are logged ");
     strcat(reply_header,"and your host machine will be permenantly banned if you exceed the maximum number of incorrect login attempts..</h2></body></html>\n");
     int opres=send(transaction->clientSock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send file as soon as we've got it

     if (opres<=0) { fprintf(stderr,"Error sending authorization needed message\n"); }
     warning("Client Denied access to resource due to being anauthorized!");
     return 0;
   }
     else
   { // Not a Bad request Start
      if ( ( transaction->incomingHeader.requestType==POST ) && (ENABLE_POST) )
       {
         fprintf(stderr,"POST HEADER : \n %s \n",transaction->incomingHeader.headerRAW);
         //TODO ADD Here a possibly rfc1867 , HTTP POST FILE compatible (multipart/form-data) recv handler..
         //TODO TODO TODO
         transaction->incomingHeader.POSTrequest = transaction->incomingHeader.headerRAW;
         transaction->incomingHeader.POSTrequestSize =  transaction->incomingHeader.ContentLength;

         fprintf(stderr,"Found a POST query %u bytes long , %s \n",transaction->incomingHeader.POSTrequestSize, transaction->incomingHeader.POSTrequest);
         warning("Will now pretend that we are a GET request for the rest of the page to be served nicely until I fix it :P\n");

         //Will now pretend that we are a GET request for the rest of the page to be served nicely
         transaction->incomingHeader.requestType=GET;
       }


     if ( (transaction->incomingHeader.requestType==GET)||(transaction->incomingHeader.requestType==HEAD))
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
      if  (strcmp(transaction->incomingHeader.resource,"/")==0) { resource_is_a_directory=1; }
        else
      if (strlen(transaction->incomingHeader.resource)>0)
       {
         if (transaction->incomingHeader.resource[strlen(transaction->incomingHeader.resource)-1]=='/')
           {
             resource_is_a_directory=1;
           }
       }

      strncpy(servefile,transaction->incomingHeader.verified_local_resource,MAX_FILE_PATH);
      //servefile variable now contains just the verified_local_resource as generated by http_header_analysis.c
      //we have checked transaction->incomingHeader.verified_local_resource for .. , weird ascii characters etc, so it should be safe for usage from now on..!

      //There are some virtual files we want to re-route to their real path..!
      if (cache_ChangeRequestIfTemplateRequested(instance,servefile,instance->templates_root) )
      { //Skip disk access times for checking for directories and other stuff..!
        //We know that the resource is a file from our cache indexes..!
          resource_is_a_directory=0;
          resource_is_a_file=1;
      }


      //STEP 0 : Check with cache!

      if (cache_ResourceExists(instance,servefile,&transaction->resourceCacheID) )
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
           strcat(transaction->incomingHeader.resource,"/");
           strcat(servefile,"/");
           fprintf(stderr,"TODO: this path is a little problematic because the web browser on the client will think\n");
           fprintf(stderr,"that we are on the / directory , a location field in the response header will clarify things\n");
         }
       }

      // STEP 2 : If we are sure that we dont have a directory then we have to find out accessing disk , could it be that our client wants a file ?
      if ( (!resource_is_a_directory) && (FileExistsAmmServ(servefile)) )
       {
           resource_is_a_file=1;
       }

      // STEP 3 : If after these steps the resource turned out to be a directory , we cant serve raw directories , so we will either look for an index.html
      // and if an index file cannot be found we will generate a directory list and send that instead..!
      if (resource_is_a_directory)
           {
             /*resource_is_a_directory means we got something like directory1/directory2/ so we should check for index file at the path given..! */
             if ( FindIndexFile(instance,instance->webserver_root,transaction->incomingHeader.resource,servefile) )
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
       strncpy(servefile,instance->webserver_root,MAX_FILE_PATH);
       strncat(servefile,transaction->incomingHeader.resource,MAX_FILE_PATH);
       ReducePathSlashes_Inplace(servefile);

       unsigned long sendSize = 0;
       char * replyBody = GenerateDirectoryPage(servefile,transaction->incomingHeader.resource,&sendSize);
       if (replyBody !=0)
        {
          //If Directory_listing enabled and directory is ok , send the generated site
          SendMemoryBlockAsFile("dir.html",transaction->clientSock,replyBody ,sendSize);
          if (replyBody !=0) { free(replyBody ); }
        } else
        {
          //If Directory listing disabled or directory is not ok send a 404
          SendErrorFile(instance,transaction,404);
        }


       return 0;
       we_can_send_result=0;
     }
        else
      if  (resource_is_a_file)
      {
         //We have a specific request for a file ( transaction->incomingHeader.resource )
         fprintf(stderr,"It is a file request for %s ..!\n",servefile);
         we_can_send_result=1;
      }
       else
     {
        fprintf(stderr,"404 not found..!!\n");
        SendErrorFile(instance,transaction,404);

        return 0;
        we_can_send_result=0;
     }



      //SendFile decides about the safety of the resource requested..
      //it should deny requests to paths like ../ or /etc/passwd
      if (we_can_send_result) //This means that we have found a file to serve..!
      {
       if ( !SendFile (
                         instance,
                         transaction,
                         servefile,  // -- Filename to be served
                         0 // <- We dont want to force an error code!
                      )
          )
         {
           //We where unable to serve request , closing connections..\n
           fprintf(stderr,"We where unable to serve request , closing connections..\n");
           return 0;
         }
      }
     } else
     //It is not a valid GET / HEAD / POST request , so use the default handlers for Bad / Not Implemented requests..!

     if (transaction->incomingHeader.requestType==BAD)
     { //In case some of the security features of the server sensed a BAD! request we should log it..
       fprintf(stderr,"BAD predatory Request sensed by header analysis!");
       //TODO : call -> int ErrorLogAppend(char * IP,char * DateStr,char * Request,unsigned int ResponseCode,unsigned long ResponseLength,char * Location,char * Useragent)
       SendErrorFile(instance,transaction,400);
       return 0;
     } else
     if (transaction->incomingHeader.requestType==NONE)
     { //We couldnt find a request type so it is a weird input that doesn't seem to be HTTP based
       fprintf(stderr,"Weird unrecognized Request!");
       SendErrorFile(instance,transaction,400);
       return 0;
     } else
     { //The request we got requires not implemented functionality , so we will admit not implementing it..! :P
       warning("Not Implemented Request!\n");
       SendErrorFile(instance,transaction,501);
       return 0;
     }
   } // Not a Bad request END

    clientList_signalClientStoppedUsingResource(instance->clientList,transaction->clientListID,transaction->incomingHeader.resource); // This in order for client_list to correctly track client behaviour..!
    if (!FreeHTTPHeader(&transaction->incomingHeader))
        { fprintf(stderr,"WARNING: Could not Free HTTP request\n"); }


  if ( transaction->incomingHeader.headerRAW!=0 )
        {
          free(transaction->incomingHeader.headerRAW);
          transaction->incomingHeader.headerRAW=0;
        }

  //We are done with request!


  if (!transaction->incomingHeader.keepalive) { return 0; } // Close_connection controls the receive "keep-alive" loop
  //If we are on a keepalive streak , then go on !
  return 1;
}



void * ServeClient(void * ptr)
{



  //fprintf(stderr,"Serve Client called ..\n");
  // We first have to store the context variables we got through our struct PassToHTTPThread
  // so we first need to do that
  struct PassToHTTPThread * context = (struct PassToHTTPThread *) ptr;
  if (context->keep_var_on_stack!=1)
   {
     error("KeepVarOnStack is not properly set , this is a bug .. \n Will not serve request");
     fprintf(stderr,"Bad new thread context is pointing to %p\n",context);
     return 0;
   }


  //This is the structure that holds all the state of the current ServeClient transaction
  struct AmmServer_Instance * instance = context->instance;
  struct HTTPTransaction transaction={{0}}; // This should get free'ed once it isn't needed any more see FreeHTTPHeader call!
  int close_connection=0; // <- if this is set it means Serve Client must stop

  //memset(&transaction->incomingHeader,0,sizeof(struct HTTPHeader));
  transaction.incomingHeader.headerRAW=0;
  transaction.incomingHeader.headerRAWSize=0;

  // In order for each thread to (in theory) be able to serve a different virtual website
  // we declare the webserver_root etc here and we copy the value from the thread spawning function
  // This creates a little code clutter but it is for the best..!
  transaction.prespawnedThreadFlag=context->pre_spawned_thread;
  transaction.clientSock=context->clientsock;
  transaction.threadID = context->thread_id;


  if (!transaction.prespawnedThreadFlag)
   {
     int i = pthread_detach(instance->threads_pool[transaction.threadID]);
     if (i!=0)
     {
       switch (i)
       {
         case EINVAL : warning("While trying to detach thread , The implementation has detected that the value specified by thread does not refer to a joinable thread."); break;
         case ESRCH : warning("While trying to detach thread , No thread could be found corresponding to that specified by the given thread ID."); break;
       };
     }
   }



  fprintf(stderr,"Now signaling we are ready (%u)\n",transaction.threadID);
  context->keep_var_on_stack=2; //This signals that the thread has processed the message it received..!
  fprintf(stderr,"Passing message to HTTP thread is done (%u)\n",transaction.threadID);

  if (instance==0) { error("Serve Client called without a valid instance , stopping \n"); return 0; } else
                   { fprintf(stderr,"ServeClient instance pointing @ %p \n",instance); }

  if (!setSocketTimeouts(transaction.clientSock))
   {
    warning("Could not set timeouts , this means something weird is going on , skipping everything");
    return 0;
   }

  transaction.clientListID = findOutClientIDOfPeer(instance ,transaction.clientSock);
  //Now the real fun starts :P <- helpful comment
  if ( clientList_isClientBanned(instance->clientList,transaction.clientListID) )
  {
    SendErrorCodeHeader(transaction.clientSock,403 /*Forbidden*/,"403.html",instance->templates_root);
  } else
  { /*!START OF CLIENT IS NOT ON IP-BANNED-LIST!*/
    while ( ( ServeClientKeepAliveLoop(instance,&transaction) ) && (instance->server_running) )
    {
      fprintf(stderr,"Another KeepAlive Loop Served");
      if ( clientList_isClientBanned(instance->clientList,transaction.clientListID) )
         {
           warning("Client became banned during keep-alive\n");
           SendErrorCodeHeader(transaction.clientSock,403 /*Forbidden*/,"403.html",instance->templates_root);
           break;
         }
    }
  }

  //fprintf(stderr,"Closing Socket ..");
  close(transaction.clientSock);
  //fprintf(stderr,"closed\n");

  if (!transaction.prespawnedThreadFlag)
   { //If we are running in a prespawned thread it is wrong to count this thread as a *dynamic* one that just stopped !
     //Clear thread id handler and we can gracefully exit..! ( LOCKLESS OPERATION)
     if (instance->threads_pool[transaction.threadID]==0) { fprintf(stderr,"While exiting thread , thread_pool id[%u] is already zero.. This could be a bug ..\n",transaction.threadID); }
     instance->threads_pool[transaction.threadID]=0;
     ++instance->CLIENT_THREADS_STOPPED;

     //We also only want to stop the thread if itsnot prespawned !
     //fprintf(stderr,"Exiting Thread\n");
     //pthread_join(instance->threads_pool[transaction.threadID],0);
     pthread_exit(0);
   }

  return 0;
}

/*
  -----------------------------------------------------------------
  -----------------------------------------------------------------
          /\                                             /\
          ||          THE MAIN SERVE CLIENT CALL         ||
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

void * MainHTTPServerThread (void * ptr)
{
  struct PassToHTTPThread * context = (struct PassToHTTPThread *) ptr;
  if (context==0) { fprintf(stderr,"Error , HTTPServerThread called without a context\n"); /*We dont have a context , so we cant signal anything :P */ return 0; }


  unsigned int serverlen = sizeof(struct sockaddr_in),clientlen = sizeof(struct sockaddr_in);
  struct sockaddr_in server;
  struct sockaddr_in client;

  struct AmmServer_Instance * instance = context->instance;
  if (instance==0) { error("HTTPServerThread called with an invalid instance\n"); context->keep_var_on_stack=2;  return 0; }
  fprintf(stderr,"HTTPServerThread instance pointing @ %p \n",instance);

  int serversock = socket(AF_INET, SOCK_STREAM, 0);
    if ( serversock < 0 ) { error("Server Thread : Opening socket"); instance->server_running=0; context->keep_var_on_stack=2;  return 0; }
  instance->serversock = serversock;

  bzero(&client,clientlen);
  bzero(&server,serverlen);

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(context->port);

  childFinishedWithParentMessage(&context->keep_var_on_stack); //If we were not able to bind we still signal that we got the message so that parent thread can continue

  //We bind to our port..!
  if ( bind(serversock,(struct sockaddr *) &server,serverlen) < 0 )
    {
      error("Server Thread : Error binding master port!\nThe server may already be running ..\n");
      instance->server_running=0;
      return 0;
    }

  //MAX_CLIENT_THREADS <- this could also be used instead of MAX_CLIENTS_LISTENING_FOR
  //I am trying a larger listen queue to hold incoming connections regardless of the serving threads
  //so that they will be used later
  if ( listen(serversock,MAX_CLIENTS_LISTENING_FOR /*MAX_CLIENT_THREADS*/) < 0 )  //Note that we are listening for a max number of clients as big as our maximum thread number..!
         {
           error("Server Thread : Failed to listen on server socket");
           instance->server_running=0;
           return 0;
         }



  //If we made it this far , it means we got ourselves the port we wanted and we can start serving requests , but before we do that..
  //The next call Pre"forks" a number of threads specified in configuration.h ( MAX_CLIENT_PRESPAWNED_THREADS )
  //They can reduce latency by up tp 10ms on a Raspberry Pi , without any side effects..
  PreSpawnThreads(instance);

  while ( (instance->server_running) && (instance->stop_server==0) && (GLOBAL_KILL_SERVER_SWITCH==0) )
  {
    fprintf(stderr,"\nServer Thread : Waiting for a new client\n");
    /* Wait for client connection */
    int clientsock=0;
    if ( (clientsock = accept(serversock,(struct sockaddr *) &client, &clientlen)) < 0) { error("Server Thread : Failed to accept client connection"); }
      else
      {
           fprintf(stderr,"Server Thread : Accepted new client , now deciding on prespawned vs freshly spawned.. \n");

           if (UsePreSpawnedThreadToServeNewClient(instance,clientsock,client,clientlen,instance->webserver_root,instance->templates_root))
            {
              // This request got served by a prespawned thread..!
              // Nothing to do here , proceeding to the next incoming connection..
              // if we failed to use a pre spawned thread we will spawn a new one using the next call..!
            } else
            if (SpawnThreadToServeNewClient(instance,clientsock,client,clientlen,instance->webserver_root,instance->templates_root))
            {
              // This request got served by a freshly spawned thread..!
              // Nothing to do here , proceeding to the next incoming connection..
              // if we failed then nothing can be done for this client
            } else
            {
                warning("Server Thread : We dont have enough resources to serve client\n");
                close(clientsock);
            }

      }
 }
  instance->server_running=0;
  instance->stop_server=2;

  warning("Server Stopped..");
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

  instance->clientList = clientList_initialize();

  int retres=0;
  volatile struct PassToHTTPThread context={ { 0 } };
  //memset((void*) &context,0,sizeof(context));
   context.port=port;
   context.instance = instance; //Also pass instance on new thread..
   context.keep_var_on_stack=1;

   instance->server_running=1;
   instance->pause_server=0;
   instance->stop_server=0;
   strncpy((char*) instance->webserver_root,root_path,MAX_FILE_PATH);
   strncpy((char*) instance->templates_root,templates_path,MAX_FILE_PATH);
  //strncpy((char*) context.ip,ip,MAX_IP_STRING_SIZE);


   fprintf(stderr,"StartHTTPServer instance pointing @ %p \n",instance);
   /*
   pthread_attr_init(&instance->attr);
   size_t stacksize;
   pthread_attr_getstacksize(&instance->attr, &stacksize);
   error("Setting Stack Size");
   fprintf(stderr,"pthread_attr_getstacksize(%u , %u MB )\n",stacksize, (stacksize/(1024*1024)) );

   stacksize = 16  * 1024 * 1024 ;
   fprintf(stderr,"pthread_attr_setstacksize(%u , %u MB )\n",stacksize, (stacksize/(1024*1024)) );
   pthread_attr_setstacksize(&instance->attr, stacksize);
   pthread_attr_setdetachstate(&instance->attr, PTHREAD_CREATE_DETACHED );
   */

  //Creating the main WebServer thread..
  //It will bind the ports and start receiving requests and pass them over to new and prespawned threads
   pthread_t server_thread_id;
   retres = pthread_create( &server_thread_id , 0 /*&instance->attr*/ ,MainHTTPServerThread,(void*) &context);
   instance->server_thread_id = server_thread_id;
   //If pthread_creation was a success, we wait for the new thread to get its configuration parameters..

   if ( retres==0 )
     {
         //parentKeepMessageOnStackUntilReady(&context.keep_var_on_stack); <-- This does not work on ARM devices , there is some problem
         while (context.keep_var_on_stack==1) { usleep(100); /*wait;*/ fprintf(stderr,"."); } //<- it is ok to print a little stuff here , it only happens once
      }

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

  clientList_close(instance->clientList);

  //pthread_attr_destroy(&instance->attr);

  return (instance->stop_server==2);
}


