
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "prespawnedThreads.h"
#include "freshThreads.h"
#include "clientServer.h"
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
      pthread_mutex_lock(&prespawned_data->operation_mutex);

      if (!prespawned_data->busy)
       {
          struct timespec ts;
          clock_gettime(CLOCK_REALTIME,&ts);
          ts.tv_nsec += 200000000; // ~200ms deadline
          if (ts.tv_nsec>=1000000000) { ts.tv_sec++; ts.tv_nsec-=1000000000; }
          pthread_cond_timedwait(&prespawned_data->condition_var,&prespawned_data->operation_mutex,&ts);
          // EINTR/ETIMEDOUT/spurious wakeups are all absorbed by re-checking busy below
       }

      if (!prespawned_data->busy)
       {
          pthread_mutex_unlock(&prespawned_data->operation_mutex);
          continue;
       }

      ++instance->prespawn_jobs_started;
      /*We have something to do , lets fill our context..*/
       context.instance=instance;
       context.clientsock=prespawned_data->clientsock;
       context.client=prespawned_data->client;
       context.clientlen=prespawned_data->clientlen;
       context.is_ssl_connection=prespawned_data->is_ssl_connection;
       context.pre_spawned_thread = 1; // THIS IS A !!!!PRE SPAWNED!!!! THREAD
       context.keep_var_on_stack=1;

       pthread_mutex_unlock(&prespawned_data->operation_mutex);
       // busy stays 1 until ServeClient actually returns below: freeing it earlier would let the master hand this
       // same slot a second connection while we're still blocked in a long keep-alive session, and since we're the
       // only thread watching this slot that second connection would never get served until our current one ends.

        //ServeClient from this thread ( without forking..! )
        #if DEBUG_MESSAGES
        fprintf(stderr,"Prespawned thread %u/%u starting to serve new client\n",i,MAX_CLIENT_PRESPAWNED_THREADS);
        #endif // DEBUG_MESSAGES
          ServeClientAfterUnpackingThreadMessage((void *)  &context);
        #if DEBUG_MESSAGES
        fprintf(stderr,"Prespawned thread %u/%u finished serving new client\n",i,MAX_CLIENT_PRESPAWNED_THREADS);
        #endif // DEBUG_MESSAGES
        //---------------------------------------------------

       pthread_mutex_lock(&prespawned_data->operation_mutex);
       prespawned_data->busy=0; // <- Now we are actually free for new work
       pthread_mutex_unlock(&prespawned_data->operation_mutex);

       ++instance->prespawn_jobs_finished;
  } // while the server doesn't stop..

  return 0;
}

void PreSpawnThreads(struct AmmServer_Instance * instance)
{
  if (instance==0) { errorID(ASV_ERROR_INSTANCE_NOT_ALLOCATED); }

  if ( MAX_CLIENT_PRESPAWNED_THREADS == 0 )
  {
    //We now emmit this warning one time in log.c in EmmitPossibleConfigurationWarnings, no need to spam it again and again
    //warningID(ASV_WARNING_NO_PRESPAWNED_THREADS);
    return;
  }
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

      pthread_mutex_init(&prespawned_data->operation_mutex,0);
      pthread_cond_init(&prespawned_data->condition_var,0);

      int retres = pthread_create(&prespawned_data->thread_id,0,PreSpawnedThread,(void*) &context );
      if ( retres==0 ) { while (context.i_adapt==i) { usleep(1); } } // <- Keep i value the same for long enough without locks
   }
}


int UsePreSpawnedThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root,int is_ssl_connection)
{
  if ( (instance==0) || (clientsock==0) || (clientlen==0) || (webserver_root==0) || (templates_root==0) )
                           { errorID(ASV_ERROR_INSTANCE_NOT_ALLOCATED); return 0; }

  if ( MAX_CLIENT_PRESPAWNED_THREADS == 0 )
  {
    //We now emmit this warning one time in log.c in EmmitPossibleConfigurationWarnings, no need to spam it again and again
    //warningID(ASV_WARNING_NO_PRESPAWNED_THREADS);
    return 0;
  }
   //Please note that this must only get called from the main process/thread..
   //fprintf(stderr,"UsePreSpawnedThreadToServeNewClient instance pointing @ %p \n",instance);

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
        unsigned int candidate = instance->prespawn_turn_to_serve;
        prespawned_data = (struct PreSpawnedThread *)  &prespawned_pool[candidate];

        pthread_mutex_lock(&prespawned_data->operation_mutex);
        if (prespawned_data->busy)
         {
            pthread_mutex_unlock(&prespawned_data->operation_mutex);

            //Attempt to find another prespawned context
            unsigned int i=0;
            prespawned_data=0;
            for (i=0; i<MAX_CLIENT_PRESPAWNED_THREADS; i++)
            {
              struct PreSpawnedThread * candidate_data = (struct PreSpawnedThread *) &prespawned_pool[i];
              pthread_mutex_lock(&candidate_data->operation_mutex);
              if (!candidate_data->busy) { candidate=i; prespawned_data=candidate_data; break; }
              pthread_mutex_unlock(&candidate_data->operation_mutex);
            }
         }

        if (prespawned_data==0)
         {
            fprintf(stderr,"Seems that the prespawned thread is still busy (  %u/%u ) ..\n",instance->prespawn_turn_to_serve,MAX_CLIENT_PRESPAWNED_THREADS);
            return 0;
         }

        {
             fprintf(stderr,"Decided to use prespawned thread %u/%u to serve new client\n",candidate,MAX_CLIENT_PRESPAWNED_THREADS);
             prespawned_data->clientsock=clientsock;
             prespawned_data->client=client;
             prespawned_data->clientlen=clientlen;
             prespawned_data->is_ssl_connection=is_ssl_connection;
             strncpy(prespawned_data->webserver_root,webserver_root,MAX_FILE_PATH);
             strncpy(prespawned_data->templates_root,templates_root,MAX_FILE_PATH);
             // The busy byte gets filled in last because it is what causes the client thread to wake up..!
             prespawned_data->busy=1;

             pthread_cond_signal(&prespawned_data->condition_var);
             pthread_mutex_unlock(&prespawned_data->operation_mutex);

             instance->prespawn_turn_to_serve = candidate+1;

              if ( MAX_CLIENT_PRESPAWNED_THREADS > 0 )
               {
                instance->prespawn_turn_to_serve = instance->prespawn_turn_to_serve % MAX_CLIENT_PRESPAWNED_THREADS; // <- Round robin next thread..
               }
             return 1;
         }
    }
    /* else
    {
        fprintf(stderr,"All prespawned threads are busy.. ( start %u , end %u , max %u) \n",instance->prespawn_jobs_started,instance->prespawn_jobs_finished,MAX_CLIENT_PRESPAWNED_THREADS);
    }*/
  return 0;
}

unsigned int getActivePrespawnedThreads(struct AmmServer_Instance * instance)
{
    return 0;
}
