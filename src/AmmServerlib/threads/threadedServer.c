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
#include "../network/sendHTTPHeader.h"
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
          ||                                             ||
          ||            MAIN HTTP SERVER THREAD          ||
          \/                                             \/
  -----------------------------------------------------------------
  -----------------------------------------------------------------

*/


#if WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR
static int signalChildFinishedWithParentMessageLocal(volatile int * childSwitch)
{
    *childSwitch=2;
    if (*childSwitch!=2) { error("WTF , i just changed the child switch"); return 0; }
    return 1;
}
#endif // WORKAROUND_REALLOCATION_GCC_ERROR


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

  unsigned int bindingPort = context->port;
  char bindingIP[MAX_IP_STRING_SIZE]={0};
  strncpy(bindingIP,context->ip,MAX_IP_STRING_SIZE);

  server.sin_family = AF_INET;/* set the type of connection to TCP/IP */
  server.sin_port = htons(context->port);     /* set the server port number */
   //server.sin_addr.s_addr = INADDR_ANY;   or  server.sin_addr.s_addr = htonl(INADDR_ANY); <- which of the 2 is correct ?
  if ( strlen(bindingIP)==0 )             {
                                            fprintf(stderr,"Trying to bind to INADDR_ANY\n");
                                            server.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
                                          } else
  if ( strcmp(bindingIP,"0.0.0.0")==0 )   {
                                            fprintf(stderr,"Trying to bind to INADDR_ANY\n");
                                            server.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
                                          } else
                                          {
                                            fprintf(stderr,"Trying to bind to %s:%u ",bindingIP,bindingPort);
                                            server.sin_addr.s_addr = inet_addr(bindingIP);
                                          }

  #if WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR
    signalChildFinishedWithParentMessageLocal(&context->keep_var_on_stack); //If we were not able to bind we still signal that we got the message so that parent thread can continue
  #else
   signalChildFinishedWithParentMessage(&context->keep_var_on_stack); //If we were not able to bind we still signal that we got the message so that parent thread can continue
  #endif // WORKAROUND_REALLOCATION_GCC_ERROR


  //We bind to our port..!
  //While this is relatively straight forward ( a.k.a. one bind(...) call some times between subsequent restarts the socket cannot be binded and also there are issues with permissions
  //needed for binding ports lower than a thousand ..! , so we try to sense all of these things here..
  // -------------------------------------------------------------------------------------------------------------
  unsigned int bindTries = 0;

  //SO_REUSEADDR allows to bind an address that previously had TCP connections stuck at TIME_WAIT , we definately want to have this
  //since this allows subsequent runs of the web server
  int yes = 1;
  if ( setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0  )
      {  warning("Error setting SO_REUSEADDR socket ( does the OS not support it ? ), this might cause problems rebinding.."); }

  fprintf(stderr,"Binding (%s:%u) / pid %u uid %u.. \n",bindingIP,bindingPort,getpid(),getuid());
  while (
          ( bind(serversock,(struct sockaddr *) &server,serverlen) < 0 ) &&
          ( bindTries < MAX_TRIES_TO_BIND_TO_PORT )
        )
  {
      usleep(DELAY_TRY_BINDING_TO_PORT);
      ++bindTries;
      fprintf(stderr,"Try to bind web server (%s:%u) failed , try %u/%u \n",bindingIP,bindingPort,bindTries,MAX_TRIES_TO_BIND_TO_PORT);
  }

   if (bindTries>=MAX_TRIES_TO_BIND_TO_PORT)
     {
       if ( (getuid()!=0) && (bindingPort<1000) )
              { error("UNIX does not allow binding a port below 1000 with a non root UID.. \n We tried to do it though.. \n Run with sudo or bind to a port higher than 1000 "); } else
              { error("Server Thread : Error binding master port!\nThe server may already be running ..\n");                          }
      instance->server_running=0;
      return 0;
    }
  // -------------------------------------------------------------------------------------------------------------
  // Done binding ..
  // -------------------------------------------------------------------------------------------------------------

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
  #if MAX_CLIENT_PRESPAWNED_THREADS
   PreSpawnThreads(instance);
  #endif // MAX_CLIENT_PRESPAWNED_THREADS

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
            if (SpawnThreadToServeNewClient(instance,clientsock,client,clientlen))
            {
              // This request got served by a freshly spawned thread..!
              // Nothing to do here , proceeding to the next incoming connection..
              // if we failed then nothing can be done for this client
            } else
            {
                error("Server Thread : We dont have enough resources to serve client\n");
                close(clientsock);
            }

      }
    usleep(10); fprintf(stderr,".-.");
 }
  instance->server_running=0;
  instance->stop_server=2;

  warning("Server Stopped..");
  //It should already be closed so skipping this : close(serversock);
  //pthread_exit(0);

  //This should make the thread release all of its resources (?)
  pthread_detach(pthread_self());
  return 0;
}




int StartHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path)
{
  if (instance==0) { fprintf(stderr,"StartHTTPServer called with an unallocated instance \n"); return 0; }

  //Since this webserver is "serious-stuff" we may want to increase its priority..
   if ( CHANGE_PRIORITY != 0 )
    { if ( nice(CHANGE_PRIORITY) == -1 ) { fprintf(stderr,"Error changing process priority to %i \n",CHANGE_PRIORITY); } else
                                         { fprintf(stderr,"Changed priority to %i \n",CHANGE_PRIORITY); } }
  //-------------------------------------------------------------------------------------------------------------

  instance->clientList = clientList_initialize();

  int retres=0;
  volatile struct PassToHTTPThread context={ 0 };
  //memset((void*) &context,0,sizeof(context));

   context.port=port;
   context.instance = instance; //Also pass instance on new thread..
   context.keep_var_on_stack=1;

   context.ip[0]=0;
   if (ip!=0) { strncpy((char*) context.ip , ip , MAX_IP_STRING_SIZE); }

   instance->server_running=1;
   instance->pause_server=0;
   instance->stop_server=0;

   strncpy((char*) instance->webserver_root,root_path,MAX_FILE_PATH);
   strncpy((char*) instance->templates_root,templates_path,MAX_FILE_PATH);


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
   retres = pthread_create( &instance->server_thread_id , 0 /*&instance->attr*/ ,MainHTTPServerThread,(void*) &context);

   //If pthread_creation was a success, we wait for the new thread to get its configuration parameters..

   if ( retres==0 )
     {
         //parentKeepMessageOnStackUntilReady(&context.keep_var_on_stack); <-- This does not work on ARM devices , there is some problem
         while (context.keep_var_on_stack==1)
         {
           usleep(THREAD_SLEEP_TIME_WHILE_WAITING_FOR_NEW_CREATED_THREAD_TO_CONSUME_PARAMETERS); /*wait;*/
           fprintf(stderr,".");
         } //<- it is ok to print a little stuff here , it only happens once
      }

   //We flip the retres
   if (retres!=0) retres = 0; else retres = 1;

   //After changing our priority , binding our port ( which could be 80 and could require superuser powers ) , it may be time to drop RootUID
   //There could be a user like www-run or something else that we could setuid to , but this isn't yet implemented...
   ServerThreads_DropRootUID();

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
  pthread_cancel(instance->server_thread_id); //This should try to kill the main thread

  return (instance->stop_server==2);
}


unsigned int GetActiveHTTPServerThreads(struct AmmServer_Instance * instance)
{
  return getActivePrespawnedThreads(instance) + getActiveFreshThreads(instance);
}
