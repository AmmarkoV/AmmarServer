#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED

#include "state.h"

void * dashboard_callback(struct AmmServer_DynamicRequest * rqst);
void * createProject_callback(struct AmmServer_DynamicRequest * rqst);
void * share_callback(struct AmmServer_DynamicRequest * rqst);

#endif // PROJECT_H_INCLUDED
