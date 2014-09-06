
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prespawnedThreads.h"
#include "freshThreads.h"
#include <pthread.h>
#include <unistd.h>
#include "../threads/threadedServer.h"
#include "../tools/logs.h"
#include "../AmmServerlib.h"



struct PassToPreSpawnedThread
{
    struct AmmServer_Instance * instance;
    unsigned int i_adapt;
};


void * PreSpawnedThread(void * ptr)
{
  //We are a thread so lets retrieve our variables..
  struct PassToPreSpawnedThread * incoming_context = (struct PassToPreSpawnedThread *) ptr;

  struct AmmServer_Instance * instance = incoming_context->instance;
  unsigned int i = incoming_context->i_adapt;
  incoming_context->i_adapt = MAX_CLIENT_PRESPAWNED_THREADS+1; // <-- This signals we got the i value..


  if (instance==0) { fprintf(stderr,"Prespawned thread did not receive a valid instance context\n"); return 0; }
  //We will also spawn our own threads so lets prepare their variables..
  volatile struct PassToHTTPThread context={0}; // <-- This is the static copy of the context we will pass through
  //memset((void*)&context,0,sizeof(struct PassToHTTPThread)); // We clear it out


  struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
  struct PreSpawnedThread * prespawned_data;
  prespawned_data = (struct PreSpawnedThread *) &prespawned_pool[i];

  while ( (instance->stop_server==0) && (GLOBAL_KILL_SERVER_SWITCH==0) )
   {
      //fprintf(stderr,"Thread %u is now waiting\n",prespawned_data->threadNum);
      //pthread_cond_wait(&prespawned_data->condition_var,&prespawned_data->operation_mutex);
      //fprintf(stderr,"Thread %u is now executing\n",prespawned_data->threadNum);

      //fprintf(stderr,"Prespawned Thread %u waiting ( its %u's turn ) \n",i,prespawn_turn_to_serve);
      //fprintf(stderr,"Prespawned Thread %u busy status %u \n",i,(unsigned int) prespawned_data->busy);
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
             context.keep_var_on_stack=1;

              //ServeClient from this thread ( without forking..! )
              fprintf(stderr,"Prespawned thread %u/%u starting to serve new client\n",i,MAX_CLIENT_PRESPAWNED_THREADS);
                ServeClient((void *)  &context);
              fprintf(stderr,"Prespawned thread %u/%u finished serving new client\n",i,MAX_CLIENT_PRESPAWNED_THREADS);
              //---------------------------------------------------

             prespawned_data->busy=0; // <- This signals we finished our task ..!
             ++instance->prespawn_jobs_finished;


             //pthread_mutex_lock(&prespawned_data->operation_mutex);
           }

      if (instance->prespawn_turn_to_serve==i)
            { usleep(THREAD_SLEEP_TIME_WHEN_OUR_PRESPAWNED_THREAD_IS_NEXT); /*It is our turn next so lets stay vigilant ( But not use a crazy lot of CPU time ) */ }  else
               { usleep(THREAD_SLEEP_TIME_FOR_PRESPAWNED_THREADS); /*It is not our turn so lets chill for more time..*/ }
  } // while the server doesn't stop..

  return 0;
}

void PreSpawnThreads(struct AmmServer_Instance * instance)
{
  if (instance==0) { error("Cannot UsePreSpawnedThreadToServeNewClient without a valid instance"); }

  #if MAX_CLIENT_PRESPAWNED_THREADS==0
    warning("PreSpawning Threads is disabled , alter MAX_CLIENT_PRESPAWNED_THREADS to enable it..\n");
  #else
  if ( (instance==0)||(instance->prespawned_pool==0) ) { fprintf(stderr,"PreSpawnThreads called on an invalid instance..\n"); return; }

  struct PassToPreSpawnedThread context={0};
  //memset(&context,0,sizeof(struct PassToPreSpawnedThread));

  struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
  struct PreSpawnedThread * prespawned_data=0;

  unsigned int i=0;
  for (i=0; i<MAX_CLIENT_PRESPAWNED_THREADS; i++)
   {
      prespawned_data = (struct PreSpawnedThread *) &prespawned_pool[i];

      context.instance = instance;
      context.i_adapt = i;
      prespawned_data->busy=0; // We do this here (and not in the PreSpawnedThread ) to make sure a clean state is sure to be initialized , not having race conditions , locks etc...
      prespawned_data->threadNum=i;

      //pthread_mutex_init(&prespawned_data->operation_mutex,0);
      //pthread_cond_init(&prespawned_data->condition_var,0);

      //pthread_mutex_lock(&prespawned_data->operation_mutex);
      int retres = pthread_create(&prespawned_data->thread_id,0,PreSpawnedThread,(void*) &context );
      if ( retres==0 ) { while (context.i_adapt==i) { usleep(1); } } // <- Keep i value the same for long enough without locks
   }
  #endif // MAX_CLIENT_PRESPAWNED_THREADS activated..
}


int UsePreSpawnedThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root)
{
  if ( (instance==0) || (clientsock==0) || (clientlen==0) || (webserver_root==0) || (templates_root==0) )
                           { error("Cannot UsePreSpawnedThreadToServeNewClient without a valid instance"); return 0; }

  #if MAX_CLIENT_PRESPAWNED_THREADS==0
    warning("PreSpawning Threads is disabled , alter MAX_CLIENT_PRESPAWNED_THREADS to enable it..\n");
    fprintf(stderr,"Was trying to bind to port %u \n" ,client.sin_port);
    return 0;
  #else
   //Please note that this must only get called from the main process/thread..
   fprintf(stderr,"UsePreSpawnedThreadToServeNewClient instance pointing @ %p \n",instance);

   struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
   struct PreSpawnedThread * prespawned_data=0;


   /* This doesnt work as it was supposed to!
   if ( instance->prespawn_jobs_started < instance->prespawn_jobs_finished )
   {
       warning("Prespawn jobs counters truncated (?) \n");
       fprintf(stderr,"Prespawn Trunc Details ( start %u , end %u , max %u) \n",instance->prespawn_jobs_started,instance->prespawn_jobs_finished,MAX_CLIENT_PRESPAWNED_THREADS);
    } else
    */
  // if This doesnt work as it was supposed to : (instance->prespawn_jobs_started-instance->prespawn_jobs_finished<MAX_CLIENT_PRESPAWNED_THREADS)
    {
        if (instance->prespawn_turn_to_serve>=MAX_CLIENT_PRESPAWNED_THREADS) { instance->prespawn_turn_to_serve=0; }
        prespawned_data = (struct PreSpawnedThread *)  &prespawned_pool[instance->prespawn_turn_to_serve];

        //Attempt to find another prespawned context
        if (prespawned_data->busy)
         {
            unsigned int i=0;
            for (i=0; i<MAX_CLIENT_PRESPAWNED_THREADS; i++)
            {
              prespawned_data = (struct PreSpawnedThread *) &prespawned_pool[i];
              if (!prespawned_data->busy) { break; }
            }
         }

        if (prespawned_data->busy)
         {
            fprintf(stderr,"Seems that the prespawned thread is still busy (  %u/%u ) ..\n",instance->prespawn_turn_to_serve,MAX_CLIENT_PRESPAWNED_THREADS);
            return 0;
         }

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


             //pthread_mutex_unlock(&prespawned_data->operation_mutex);
             fprintf(stderr,"Thread %u is now unlocked\n",prespawned_data->threadNum);

             ++instance->prespawn_turn_to_serve;
             #if MAX_CLIENT_PRESPAWNED_THREADS > 0
              instance->prespawn_turn_to_serve = instance->prespawn_turn_to_serve % MAX_CLIENT_PRESPAWNED_THREADS; // <- Round robin next thread..
             #endif

             return 1;
         }
    }
    /* else
    {
        fprintf(stderr,"All prespawned threads are busy.. ( start %u , end %u , max %u) \n",instance->prespawn_jobs_started,instance->prespawn_jobs_finished,MAX_CLIENT_PRESPAWNED_THREADS);
    }*/
  return 0;
  #endif // MAX_CLIENT_PRESPAWNED_THREADS not zero..
}

