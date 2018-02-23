#include "login.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct UserAccountDatabase * uadb = 0;

int initializeLoginSystem()
{
  uadb = uadb_initializeUserAccountDatabase("db/users.db");
  return (uadb!=0);
}


int stopLoginSystem()
{
  uadb_closeUserAccountDatabase(&uadb);
  return 1;
}



void  * login_callback(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int haveName=0,havePass=0;
  char name[128]={0};
  if ( _GET(rqst,"name",name,128) )     { haveName=1; }

  char pass[128]={0};
  if ( _GET(rqst,"password",pass,512) ) { havePass=1; }

  struct UserAccountAuthenticationToken outputToken={0};

  if ((haveName)&&(havePass))
  {
  if
     (
      uadb_loginUser(uadb,
                     &outputToken,
                     name,
                     pass,
                     ENCODING_PLAINTEXT,
                     "0.0.0.0",
                     "no fingerprint"
                    )
     )
     {
      snprintf(rqst->content,rqst->MAXcontentSize,
               "<html>\
                 <head>\
                  <title>Logging in</title>\
                   <meta http-equiv=\"refresh\" content=\"0; url=home.html?s=%s\">\
                 </head>\
                 <body>Success loging in</body>\
                </html>",
               outputToken.sessionID); //Success
     } else
     {
      snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Wrong Username/Password combination</body></html>");  //Wrong Password / No user
     }
  }  else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Bad Request</body></html>");  //Bad request
  }

 rqst->contentSize=strlen(rqst->content);
 return 0;
}
