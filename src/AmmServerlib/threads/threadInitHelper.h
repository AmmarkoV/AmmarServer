#ifndef THREADINITHELPER_H_INCLUDED
#define THREADINITHELPER_H_INCLUDED


int parentKeepMessageOnStackUntilReadyOrTimeout(volatile int * childSwitch,unsigned int maxWaitTime);
int parentKeepMessageOnStackUntilReady(volatile int * childSwitch);
void childFinishedWithParentMessage(volatile int * childSwitch);

#endif // THREADINITHELPER_H_INCLUDED
