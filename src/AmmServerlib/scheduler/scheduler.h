/** @file scheduler.h
* @brief Scheduling functions for applications that need to perform programmable maintenance tasks..
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef SCHEDULER_H_INCLUDED
#define SCHEDULER_H_INCLUDED


#include "../AmmServerlib.h"

 int schedulerAdd(
                  const char * resource_name ,
                  void * callback,
                  unsigned int delayMilliseconds,
                  unsigned int repetitions
                  );

 int schedulerFlush();


#endif // LOGS_H_INCLUDED
