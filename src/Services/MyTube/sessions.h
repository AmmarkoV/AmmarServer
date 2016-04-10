#ifndef SESSIONS_H_INCLUDED
#define SESSIONS_H_INCLUDED


#include "indexer.h"
#include "../../AmmServerlib/AmmServerlib.h"

unsigned int getAUserIDForSession(struct videoCollection * db , const char * sessionQuery , const char * sessionToken , int * foundSession);

#endif // SESSIONS_H_INCLUDED
