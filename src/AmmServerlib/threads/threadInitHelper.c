#include "threadInitHelper.h"

#include "../tools/logs.h"
//#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "../server_configuration.h"

#define SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE 10

static int parentKeepMessageOnStackUntilReadyOrTimeout(volatile int * childSwitch,unsigned int maxWaitTime)
{
    usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
    if (*childSwitch!=1) { return 1; }
    usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
    if (*childSwitch!=1) { return 1; }

    unsigned int countSleepTime=0;
    while (*childSwitch!=1)
         {
           usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
           ++countSleepTime;
           if (countSleepTime>maxWaitTime) { return 0; }
         }

    return 1;
}

static int parentKeepMessageOnStackUntilReady(volatile int * childSwitch)
{
    usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
    if (*childSwitch!=1) { return 1; }
    usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
    if (*childSwitch!=1) { return 1; }

    while (*childSwitch!=1)
         {
           fprintf(stderr,"?");
           usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
         }

    return 1;
}

static void signalChildFinishedWithParentMessage(volatile int * childSwitch)
{
    #if WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR
      #warning "Using a workaround that does not call signalChildFinishedWithParentMessage due to an unsolvable reallocation error"
    #endif // WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR

    *childSwitch=2;
    if (*childSwitch!=2) { error("WTF , i just changed the child switch"); return; }
    return;
}
