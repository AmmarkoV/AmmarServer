#include "login.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void  * login_callback(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int haveName=0,havePass=0;
  char name[128]={0};
  if ( _GET(rqst->instance,rqst,"name",name,128) )     { haveName=1; }

  char pass[128]={0};
  if ( _GET(rqst->instance,rqst,"password",pass,512) ) { havePass=1; }
}
