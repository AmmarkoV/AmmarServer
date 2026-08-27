#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"
#include "state.h"

void * prepareThreadView(struct AmmServer_DynamicRequest  * rqst);
void * prepareThreadIndexView(struct AmmServer_DynamicRequest  * rqst);


int addThreadToBoard(const char * boardName , const char * threadName);

int saveThreadStatus(const char * boardName , struct thread * ourThread);

int createThread(
                   const char * boardName ,
                   const char * op ,
                   const char * title ,
                   const char * password ,
                   struct post * opPost ,
                   const char * fileBytes ,
                   unsigned int fileBytesSize ,
                   char * outThreadName ,
                   unsigned int outThreadNameSize
                 );

#endif // THREAD_H_INCLUDED
