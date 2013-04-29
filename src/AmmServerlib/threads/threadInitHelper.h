#ifndef THREADINITHELPER_H_INCLUDED
#define THREADINITHELPER_H_INCLUDED


#define THREAD_INITIALIZATION_HASNT_STARTED 0
#define THREAD_INITIALIZATION_IS_RUNNING 1
#define THREAD_INITIALIZATION_SUCCESSF 2
#define THREAD_INITIALIZATION_FAILED 3


int parentKeepMessageOnStackUntilReadyOrTimeout(volatile int * childSwitch,unsigned int maxWaitTime);
int parentKeepMessageOnStackUntilReady(volatile int * childSwitch);
void childFinishedWithParentMessage(volatile int * childSwitch);

#endif // THREADINITHELPER_H_INCLUDED
