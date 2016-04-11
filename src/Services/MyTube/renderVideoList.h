#ifndef RENDERVIDEOLIST_H_INCLUDED
#define RENDERVIDEOLIST_H_INCLUDED


#include "indexer.h"
#include "../../AmmServerlib/AmmServerlib.h"

int renderVideoList(struct videoCollection *  db , struct AmmServer_MemoryHandler * headerHTML , struct AmmServer_DynamicRequest  *  rqst , const char * query , unsigned int userID , unsigned int * doImmediateVideoID);

#endif // RENDERVIDEOLIST_H_INCLUDED
