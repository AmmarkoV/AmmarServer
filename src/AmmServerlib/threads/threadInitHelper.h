/** @file threadInitHelper.h
* @brief Helper Functions to help with passing messages around ..
*
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef THREAD_INITHELPER_H_INCLUDED
#define THREAD_INITHELPER_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif


#include "../tools/logs.h"
//#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "../server_configuration.h"

#define SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE 10


/**
* @brief A call to help waiting for a message to be consumed by a child thread , or until a timeout , this should be called by the parent thread
* @ingroup threads
* @param Pointer to the switch signaling that the child has read the message , so that the parent will keep it in his stack
* @param Maximum time to wait before terminating the wait to ensure no dead-locks
* @retval 1=Success,0=Fail */
static int parentKeepMessageOnStackUntilReadyOrTimeout(volatile int * childSwitch,unsigned int maxWaitTime)
{
    usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
    if (*childSwitch!=1) { return 1; }
    usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
    if (*childSwitch!=1) { return 1; }

    unsigned int countSleepTime=0;
    while (*childSwitch!=1)
         {
           fprintf(stderr,"?");
           usleep(SLEEP_FOR_N_NANOSECONDS_WAITING_STACK_MESSAGE);
           ++countSleepTime;
           if (countSleepTime>maxWaitTime) { return 0; }
         }

    return 1;
}


/**
* @brief A call to help waiting for a message to be consumed by a child thread , this should be called by the parent thread
* @ingroup threads
* @param Pointer to the switch signaling that the child has read the message , so that the parent will keep it in his stack
* @retval 1=Success,0=Fail
* @bug For some reason this doesnt have the desired effect :S there is a bug here..
*/
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

/**
* @brief A call to help waiting for a message to be consumed by a child thread , this should be called by the child thread when it finished copying the message
* @ingroup threads
* @param Pointer to the switch signaling that the child has read the message , so that the parent will keep it in his stack
* @retval 1=Success,0=Fail */
static void signalChildFinishedWithParentMessage(volatile int * childSwitch)
{
    #if WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR
      #warning "Using a workaround that does not call signalChildFinishedWithParentMessage due to an unsolvable reallocation error"
    #endif // WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR

    *childSwitch=2;
    if (*childSwitch!=2) { error("WTF , i just changed the child switch"); return; }
    return;
}

#ifdef __cplusplus
}
#endif

#endif // THREADINITHELPER_H_INCLUDED
