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
  if ( _GET(rqst->instance,rqst,"s",sessionID,128) )     { haveSessionRequested=1; }

  if (haveSessionRequested)
  {
     UserAccount_UserID realUserID=0;
     if (
          uadb_getUserIDForSessionID(
                                     uadb,
                                     sessionID,
                                     &realUserID
                                    )
        )
     {
       //Can serve here..
        struct UserAccountAuthenticationToken outputToken;
        uadb_getUserTokenFromUserID(
                                    uadb,
                                    &outputToken,
                                    realUserID
                                   );


        snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>User : %s </body></html>",outputToken.username);  //Wrong Session / No user
        rqst->contentSize=strlen(rqst->content);
       return 0;
     }
  }

 snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Error with user session</body></html>");  //Wrong Session / No user
 rqst->contentSize=strlen(rqst->content);
 return 0;
}
