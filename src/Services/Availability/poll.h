#ifndef POLL_H_INCLUDED
#define POLL_H_INCLUDED

#include "state.h"

void * createPoll_callback(struct AmmServer_DynamicRequest * rqst);
void * votePage_callback(struct AmmServer_DynamicRequest * rqst);
void * submitVote_callback(struct AmmServer_DynamicRequest * rqst);
void * pollResultsFragment_callback(struct AmmServer_DynamicRequest * rqst);

void appendResultsGridHTML(char * buffer , unsigned int bufferCapacity , struct poll * p);

#endif // POLL_H_INCLUDED
