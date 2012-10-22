
#include <unistd.h>
#include "configuration.h"
#include <ctype.h>
#include <time.h>
#include "time_provider.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


unsigned long GetTickCount()
{
   struct timespec ts;
   if ( clock_gettime(CLOCK_MONOTONIC,&ts) != 0) { fprintf(stderr,"Error Getting Tick Count\n"); return 0; }
   return ts.tv_sec*1000 + ts.tv_nsec/1000000;
}



int GetDateString(char * output,char * label,unsigned int now,unsigned int dayofweek,unsigned int day,unsigned int month,unsigned int year,unsigned int hour,unsigned int minute,unsigned int second)
{
   //Date: Sat, 29 May 2010 12:31:35 GMT
   //Last-Modified: Sat, 29 May 2010 12:31:35 GMT
   if ( now )
      {
        time_t clock = time(NULL);
        struct tm * ptm = gmtime ( &clock );

        sprintf(output,"%s: %s, %u %s %u %02u:%02u:%02u GMT\n",label,days[ptm->tm_wday],ptm->tm_mday,months[ptm->tm_mon],EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

      } else
      {
        sprintf(output,"%s: %s, %u %s %u %02u:%02u:%02u GMT\n",label,days[dayofweek],day,months[month],year,hour,minute,second);
      }
    return 1;
}



int start_timer (  struct time_snap * val )
{
   return gettimeofday(&val->starttime,0x0);

}

unsigned long end_timer (  struct time_snap * val )
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
