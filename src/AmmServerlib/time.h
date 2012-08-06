#ifndef TIME_H_INCLUDED
#define TIME_H_INCLUDED

#include <sys/types.h>
#include <sys/time.h>

struct time_snap
{
  struct timeval starttime,endtime,difference;
};

int start_timer (  struct time_snap * val );
unsigned long end_timer (  struct time_snap * val );
//Notice : end_timer needs to get divided by 1000 to show msecs , and by 1000000 to show seconds..!

#endif // TIME_H_INCLUDED
