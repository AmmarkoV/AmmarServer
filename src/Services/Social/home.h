#ifndef HOME_H_INCLUDED
#define HOME_H_INCLUDED


#include "../../AmmServerlib/AmmServerlib.h"

extern struct AmmServer_MemoryHandler * homePage;

void * home_callback(struct AmmServer_DynamicRequest  * rqst);


#endif // HOME_H_INCLUDED
