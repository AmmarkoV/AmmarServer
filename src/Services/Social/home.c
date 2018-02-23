#include "home.h"
#include "login.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct AmmServer_MemoryHandler * homePage;

void * home_callback(struct AmmServer_DynamicRequest  * rqst)
{
  int haveSessionRequested=0;
  char sessionID[128]={0};
  if ( _GET(rqst,"s",sessionID,128) )     { haveSessionRequested=1; }

  if (haveSessionRequested)
  {
     struct UserAccountAuthenticationToken outputToken;
     if (
          uadb_getUserTokenFromSessionID(
                                         uadb,
                                         sessionID,
                                         &outputToken
                                        )
        )
     {
       //Can serve here..
        struct AmmServer_MemoryHandler * homeRoomWithContents = AmmServer_CopyMemoryHandler(homePage);
        AmmServer_ReplaceAllVarsInMemoryHandler(homeRoomWithContents,1,"$SESSIONID$",outputToken.sessionID);
        AmmServer_ReplaceAllVarsInMemoryHandler(homeRoomWithContents,1,"$USER$",outputToken.username);

        memcpy (rqst->content , homeRoomWithContents ->content , homeRoomWithContents ->contentCurrentLength );
        rqst->contentSize=homeRoomWithContents ->contentCurrentLength ;
        AmmServer_FreeMemoryHandler(&homeRoomWithContents);
       return 0;
     }
  }

 snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Error with user session</body></html>");  //Wrong Session / No user
 rqst->contentSize=strlen(rqst->content);
 return 0;
}
