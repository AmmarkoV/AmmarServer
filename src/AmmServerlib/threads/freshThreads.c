#include "freshThreads.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "../server_configuration.h"
#include "../network/networkAbstraction.h"
#include "../threads/clientServer.h"
#include "../threads/threadedServer.h"
#include "../tools/logs.h"
#include "threadInitHelper.h"

#define WEIRD_THING_THAT_WORKS 1
#define MAX_TRIES_TO_FIND_A_THREAD_ID 5

unsigned int FindAProperThreadID(struct AmmServer_Instance * instance,int * success)
{
    *success=0;
    unsigned int starting_from = instance->CLIENT_THREADS_STARTED;
    if (starting_from>=MAX_CLIENT_THREADS) { starting_from = starting_from % MAX_CLIENT_THREADS; }

  // fprintf(stderr,"FindAProperThreadID instance pointing @ %p \n",instance);//Clear instance..!
  //  fprintf(stderr,"FindAProperThreadID thread pool pointing @ %p \n",instance->threads_pool);//Clear instance..!
    unsigned int tries=0;

    while ( tries<MAX_TRIES_TO_FIND_A_THREAD_ID )
     {
       while (starting_from<MAX_CLIENT_THREADS)
         {
            if  (  instance->threads_pool[starting_from]==0  )
                 {
                   *success=1;
                   return starting_from;
                 }
            ++starting_from;
            fprintf(stderr,"-");
          }

       starting_from=0;
       ++tries;
     }


    warning("Could not find a proper thread id..\n");
    return starting_from;
}


int SingleThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen)
{
    struct HTTPTransaction transaction={0};
    transaction.instance=instance;
    transaction.clientSock=clientsock;
    transaction.prespawnedThreadFlag=1; //Yes this is prespawned we dont want to end it after the client goes away

    ASRV_StartSession(instance,&transaction);

     int i= ServeClientInternal(instance , &transaction);

    ASRV_StopSession(instance,&transaction);

    return i;
}




int SpawnThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen)
{
  if (instance==0) { error("Cannot SpawnThreadToServeNewClient without a valid instance value"); return 0; }
  //This Segfaults -> (inet_ntoa) fprintf(stderr,"Server Thread : Client connected: %s , %u total active threads\n", inet_ntoa(client.sin_addr),instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED);

  fprintf(stderr,"SpawnThreadToServeNewClient instance pointing @ %p \n",instance);


  if (instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED >= MAX_CLIENT_THREADS)
   {
     //This needs a little thought.. it is not "nice" to drop clients  , on the other hand what`s the point on opening and
     //maintaining an idle TCP/IP connection when we are on our limits thread-wise..
     fprintf(stderr,"We are over the limit on clients served ( %u threads ) ..\nDropping client..!\n",instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED); //inet_ntoa(client.sin_addr)
     return 0;
   }

  //I Have removed the lock here , so getting a thread_id is a little more complex ..
  int foundAvailiableThreadID=0;
  unsigned int threadID = FindAProperThreadID(instance,&foundAvailiableThreadID);

  if (!foundAvailiableThreadID)
  {
      warning("Could not find a thread ID for spawning a new thread\n");
      return 0;
  }

  //TODO : Here would be a good spot to query the client socket ip address in the client_list ..
  //We may want to keep a client for opening too many connections or ban him early on , before going through the expense
  //of creating a seperate thread for him..

  unsigned int waitCounter=0,maxWaitCounter=(unsigned int) THREAD_MAXIMUM_TIME_TO_WAIT_FOR_A_NEWLY_CREATED_THREAD_MS/THREAD_SLEEP_TIME_WHILE_WAITING_FOR_NEW_CREATED_THREAD_TO_CONSUME_PARAMETERS;


  volatile struct PassToHTTPThread context={0};

  context.keep_var_on_stack=1;

  context.clientsock=clientsock;
  context.client=client;
  context.clientlen=clientlen;
  context.pre_spawned_thread = 0; // THIS IS A !!!NEW!!! THREAD , NOT A PRESPAWNED ONE
  context.thread_id =threadID;
  context.instance = instance;

  //We could only pass pointers here
  //context.webserver_root=webserver_root;
  //context.templates_root=templates_root;


  fprintf(stderr,"Spawning a new thread %u/%u (id=%u) to serve this client , context pointing @ %p\n",instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED,MAX_CLIENT_THREADS,context.thread_id,&context);

  int retres = pthread_create(&instance->threads_pool[threadID],0/*&instance->attr*/,ServeClientAfterUnpackingThreadMessage,(void*) &context);
  //It appears that in certain high loads pthread_create stops creating new threads ..
  //A good question is why..!
  if ( retres==0 )
  {
    #if WEIRD_THING_THAT_WORKS
       while (  (context.keep_var_on_stack==1) && (waitCounter<maxWaitCounter) )
           {
             /*TODO : POTENTIAL BUG HERE ? THIS WAS OPTIMIZED OUT?*/
             fprintf(stderr,"!"); //<- Without this it crashes
             usleep(THREAD_SLEEP_TIME_WHILE_WAITING_FOR_NEW_CREATED_THREAD_TO_CONSUME_PARAMETERS);
             ++waitCounter;
            }

        if (waitCounter>=maxWaitCounter)
        {
          retres=1; //Mark this thread creation as failed
          error("Timed out while waiting for a new thread to get created and consume its message\n");
          #warning "The pthread_cancel code is not tested "
          if ( pthread_cancel(instance->threads_pool[threadID]) !=0 )
          { // http://man7.org/linux/man-pages/man3/pthread_cancel.3.html
            error("Failed to cancel new thread creation\n");
          }
        }
    #else
       parentKeepMessageOnStackUntilReady(&context.keep_var_on_stack); // <- Keep PeerServerContext in stack for long enough :P
    #endif
   }
 else //Either code failed
     { warning("Could not create a new thread..\n ");
       switch (retres)
       {
         case EAGAIN : warning("Insufficient resources to create another thread, or a system-imposed limit on the number of threads was encountered."); break;
         case EINVAL : warning("Invalid settings in attr."); break ;
         case EPERM  : warning("No permission to set the scheduling policy and parameters"); break ;
       };
     }

  if (retres!=0) { retres = 0; } else { retres = 1; }

  if (retres) { instance->CLIENT_THREADS_STARTED++; }

  return retres;
}



unsigned int getActiveFreshThreads(struct AmmServer_Instance * instance)
{
 return 0;
}
