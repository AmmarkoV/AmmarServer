#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// --------------------------------------------
#include "scheduler.h"

 int schedulerAdd(
                  const char * resource_name ,
                  void * callback,
                  unsigned int delayMilliseconds,
                  unsigned int repetitions
                  )
{
   AmmServer_Stub("Scheduler Code not implemented\n");
   return 0;
}

 int schedulerFlush()
 {
   AmmServer_Stub("Scheduler Code not implemented\n");
   return 0;
 }
