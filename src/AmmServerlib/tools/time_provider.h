/** @file time_provider.h
* @brief Timer functions
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef TIME_PROVIDER_H_INCLUDED
#define TIME_PROVIDER_H_INCLUDED

#include <sys/types.h>
#include <sys/time.h>

struct time_snap
{
  struct timeval starttime,endtime,difference;
};

/**
* @brief GetTickCount like call for functions wanting to get monotonic values in milliseconds
* @ingroup timers
* @retval Milliseconds*/
unsigned long GetTickCountAmmServ();

/**
* @brief Get a string back with date and time
* @ingroup timers
* @param   Pointer to where Output String should be stored
* @param   Pointer to Label String
* @param   Flag to control if we want to override values with the current time

* @param   Unsigned Integer Day of Week Value
* @param   Unsigned Integer Day
* @param   Unsigned Integer Month
* @param   Unsigned Integer Year

* @param   Unsigned Integer Hour
* @param   Unsigned Integer Minute
* @param   Unsigned Integer Second


* @retval 1=Success,0=Failure*/
int GetDateString(
                  char * output,
                  unsigned int maxOutput,

                  char * label,
                  unsigned int now,
                  //-------------------
                  unsigned int dayofweek,
                  unsigned int day,
                  unsigned int month,
                  unsigned int year,
                  //-------------------
                  unsigned int hour,
                  unsigned int minute,
                  unsigned int second
                );

/**
* @brief Start a timer using a time_snap structure
* @ingroup timers
* @param   time_snap structure that holds the timer data
* @retval 1=Success,0=Failure*/
int start_timer (  struct time_snap * val );


/**
* @brief End a started timer and get back the results
* @ingroup timers
* @param   time_snap structure that holds the timer data
* @retval Elapsed time since start_timer , needs to be divided by 1000 to get msecs , and by 1000000 to get seconds..*/
unsigned long end_timer (  struct time_snap * val );

#endif // TIME_H_INCLUDED
