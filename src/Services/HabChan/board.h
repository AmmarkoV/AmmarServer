#ifndef BOARD_H_INCLUDED
#define BOARD_H_INCLUDED


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"


void * prepareBoardIndexView(struct AmmServer_DynamicRequest  * rqst);


int addBoardToSite( struct site * targetSite , char * boardName );
#endif // BOARD_H_INCLUDED
