#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"

void * prepareThreadView(struct AmmServer_DynamicRequest  * rqst);
void * prepareThreadIndexView(struct AmmServer_DynamicRequest  * rqst);


int addThreadToBoard(const char * boardName , const char * threadName);

#endif // THREAD_H_INCLUDED
