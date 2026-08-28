#ifndef MANAGE_H_INCLUDED
#define MANAGE_H_INCLUDED

#include "state.h"

void * managePage_callback(struct AmmServer_DynamicRequest * rqst);
void * closePoll_callback(struct AmmServer_DynamicRequest * rqst);
void * finalizePoll_callback(struct AmmServer_DynamicRequest * rqst);

#endif // MANAGE_H_INCLUDED
