#ifndef THREADINITHELPER_H_INCLUDED
#define THREADINITHELPER_H_INCLUDED


#define THREAD_INITIALIZATION_HASNT_STARTED 0
#define THREAD_INITIALIZATION_IS_RUNNING 1
#define THREAD_INITIALIZATION_SUCCESSF 2
#define THREAD_INITIALIZATION_FAILED 3


struct threadInitPayload
{
   char initializationState;
   void * payload;
};


int initThreadAndPassMessage(struct threadInitPayload *  pld,unsigned int waitTime);

#endif // THREADINITHELPER_H_INCLUDED
