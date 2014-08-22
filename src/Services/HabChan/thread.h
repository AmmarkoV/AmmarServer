#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"


void * prepareThreadIndexView(struct AmmServer_DynamicRequest  * rqst);


#endif // THREAD_H_INCLUDED
