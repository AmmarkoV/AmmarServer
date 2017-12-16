#ifndef RENDERVIDEOPAGE_H_INCLUDED
#define RENDERVIDEOPAGE_H_INCLUDED

#include "indexer.h"
#include "../../AmmServerlib/AmmServerlib.h"

int renderVideoPage(struct videoCollection *  myTube , struct AmmServer_MemoryHandler * videoMH , unsigned int videoID , unsigned int userID, unsigned int secondsStart , unsigned int stillDownloading);

#endif // RENDERVIDEOPAGE_H_INCLUDED
