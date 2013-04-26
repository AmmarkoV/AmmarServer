#include "threadInitHelper.h"
#include "logs.h"

int parent_InitThreadAndPassMessage(struct threadInitPayload * pld,unsigned int waitTime)
{
    if (pld==0) { warning("initThread called with invalid argument \n"); }
    return 0;
}


int child_InitThreadAndPassMessage(struct threadInitPayload * pld,unsigned int waitTime)
{
    if (pld==0) { warning("initThread called with invalid argument \n"); }
    return 0;
}
