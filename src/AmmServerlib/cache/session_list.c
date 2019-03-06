#include "session_list.h"
#include "../AmmServerlib.h"
#include <stdio.h>

int sessiontList_StoreInfo(struct sessionListContext * sessionList,const char * sessionName ,const char * name ,const char * value)
{
return 0;
}

const char * sessiontList_GetInfo(struct sessionListContext * sessionList,sessionID id,const char * name)
{
    AmmServer_Stub("Session access not coded in yet..!");
}


int getSessionFromHeader(
                          struct sessionListContext * sessionList,
                          const char * connectionIP ,
                          const char * cookieValue ,
                          const char * BrowserIdentifier)
{
}


struct sessionListContext *  sessionList_initialize(const char * serverName)
{
 return 0;
}


int sessionList_close(struct sessionListContext * sessionList)
{
 return 0;
}
