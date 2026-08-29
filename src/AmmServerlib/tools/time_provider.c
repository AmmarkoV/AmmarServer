#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
// --------------------------------------------
#include "../server_configuration.h"
#include "time_provider.h"
// --------------------------------------------

const char *days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

//Every response's "Date:" header needs the current time formatted the exact same way ( HTTP only requires 1-second
//resolution ) , so instead of every request-serving thread paying for its own time()+gmtime()+snprintf ( nginx does
//this exact same thing via ngx_cached_http_time ) , one background thread refreshes a shared string twice a second
//and callers just copy it out. This also fixes a latent bug : the un-cached path called plain gmtime() ( not
//gmtime_r() ) from every serving thread concurrently - gmtime() writes through a process-wide static buffer, so
//concurrent callers could tear each other's result. Only the single updater thread touches gmtime_r() now.
static char g_cached_date_value[40]={0}; // "Sat, 29 May 2010 12:31:35 GMT" , no label
static pthread_mutex_t g_cached_date_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int g_date_cache_thread_started = 0;

static void RefreshCachedDateValue()
{
   time_t clock = time(NULL);
   struct tm tmv;
   gmtime_r(&clock,&tmv);

   char local[40];
   snprintf(local,sizeof(local),"%s, %u %s %u %02u:%02u:%02u GMT",
            days[tmv.tm_wday],tmv.tm_mday,months[tmv.tm_mon],EPOCH_YEAR_IN_TM_YEAR+tmv.tm_year,tmv.tm_hour,tmv.tm_min,tmv.tm_sec);

   pthread_mutex_lock(&g_cached_date_mutex);
   strncpy(g_cached_date_value,local,sizeof(g_cached_date_value)-1);
   pthread_mutex_unlock(&g_cached_date_mutex);
}

static void * DateCacheUpdaterThread(void * ptr)
{
   while (GLOBAL_KILL_SERVER_SWITCH==0)
   {
      RefreshCachedDateValue();
      usleep(500000); // refresh twice a second - keeps the header within the 1-second resolution HTTP requires, with margin
   }
   return 0;
}

static void EnsureDateCacheThreadStarted()
{
   if (g_date_cache_thread_started) { return; }
   //A rare double-start race here just means two updater threads briefly exist , both writing the same value - harmless.
   g_date_cache_thread_started = 1;
   RefreshCachedDateValue(); // populate before this first caller reads it, don't wait for the thread to tick
   pthread_t t;
   if (pthread_create(&t,0,DateCacheUpdaterThread,0)==0) { pthread_detach(t); }
}

unsigned long GetTickCountAmmServ()
{
   //This returns a monotnic "uptime" value in milliseconds , it behaves like windows GetTickCount() but its not the same..
   struct timespec ts;
   if ( clock_gettime(CLOCK_MONOTONIC,&ts) != 0) { fprintf(stderr,"Error Getting Tick Count\n"); return 0; }
   return ts.tv_sec*1000 + ts.tv_nsec/1000000;
}

int GetDateString(char * output,unsigned int maxOutput,char * label,unsigned int now,unsigned int dayofweek,unsigned int day,unsigned int month,unsigned int year,unsigned int hour,unsigned int minute,unsigned int second)
{
   //Date: Sat, 29 May 2010 12:31:35 GMT
   //Last-Modified: Sat, 29 May 2010 12:31:35 GMT
   if ( now )
      {
        EnsureDateCacheThreadStarted();
        char cached_copy[40];
        pthread_mutex_lock(&g_cached_date_mutex);
        strncpy(cached_copy,g_cached_date_value,sizeof(cached_copy));
        pthread_mutex_unlock(&g_cached_date_mutex);

        if (label==0)
        { snprintf(output,maxOutput,"%s\n",cached_copy); } else
        { snprintf(output,maxOutput,"%s: %s\n",label,cached_copy); }
      } else
      {
        if (label==0)
        { snprintf(output,maxOutput,"%s, %u %s %u %02u:%02u:%02u GMT\n",days[dayofweek],day,months[month],year,hour,minute,second); } else
        { snprintf(output,maxOutput,"%s: %s, %u %s %u %02u:%02u:%02u GMT\n",label,days[dayofweek],day,months[month],year,hour,minute,second); }
      }
    return 1;
}

int startTimer (  struct time_snap * val )
{
   return gettimeofday(&val->starttime,0x0);
}

unsigned long endTimer (  struct time_snap * val )
{
   struct timeval *difference=&val->difference;
   struct timeval *end_time=&val->endtime;
   struct timeval *start_time =&val->starttime;
   gettimeofday(end_time,0x0);

   struct timeval temp_diff;

   if(difference==0) { difference=&temp_diff; }

  difference->tv_sec =end_time->tv_sec -start_time->tv_sec ;
  difference->tv_usec=end_time->tv_usec-start_time->tv_usec;

  /* Using while instead of if below makes the code slightly more robust. */

  while(difference->tv_usec<0)
  {
    difference->tv_usec+=1000000;
    difference->tv_sec -=1;
  }

  return 1000000LL*difference->tv_sec+ difference->tv_usec;
}
