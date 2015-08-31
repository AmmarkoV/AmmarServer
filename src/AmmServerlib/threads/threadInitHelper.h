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

/**
* @brief A call to help waiting for a message to be consumed by a child thread , or until a timeout , this should be called by the parent thread
* @ingroup threads
* @param Pointer to the switch signaling that the child has read the message , so that the parent will keep it in his stack
* @param Maximum time to wait before terminating the wait to ensure no dead-locks
* @retval 1=Success,0=Fail */
static int parentKeepMessageOnStackUntilReadyOrTimeout(volatile int * childSwitch,unsigned int maxWaitTime);

/**
* @brief A call to help waiting for a message to be consumed by a child thread , this should be called by the parent thread
* @ingroup threads
* @param Pointer to the switch signaling that the child has read the message , so that the parent will keep it in his stack
* @retval 1=Success,0=Fail
* @bug For some reason this doesnt have the desired effect :S there is a bug here..
*/
static int parentKeepMessageOnStackUntilReady(volatile int * childSwitch);

/**
* @brief A call to help waiting for a message to be consumed by a child thread , this should be called by the child thread when it finished copying the message
* @ingroup threads
* @param Pointer to the switch signaling that the child has read the message , so that the parent will keep it in his stack
* @retval 1=Success,0=Fail */
static void signalChildFinishedWithParentMessage(volatile int * childSwitch);


#ifdef __cplusplus
}
#endif

#endif // THREADINITHELPER_H_INCLUDED
