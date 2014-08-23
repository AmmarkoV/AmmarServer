#ifndef POSTRECEIVER_H_INCLUDED
#define POSTRECEIVER_H_INCLUDED


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"


void * processPostReceiver(struct AmmServer_DynamicRequest  * rqst);



#endif // POSTRECEIVER_H_INCLUDED
