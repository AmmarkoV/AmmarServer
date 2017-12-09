#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "login.h"

char loginBody[]= {
#include "include/login.html"
};

void * login_callback(struct AmmServer_DynamicRequest  * rqst)
{
 snprintf(rqst->content,rqst->MAXcontentSize,"%s",loginBody);
 rqst->contentSize=strlen(rqst->content);
 return 0;
}
