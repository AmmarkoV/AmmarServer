
#include <unistd.h>
#include "time.h"

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
