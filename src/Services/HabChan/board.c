

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "state.h"

#include "../../AmmServerlib/AmmServerlib.h"


//This function prepares the content of  stats context , ( stats.content )
void * prepareBoardIndexView(struct AmmServer_DynamicRequest  * rqst)
{
    strcpy(rqst->content,"<html><body>Welcome to Hab Chan<br>");

    unsigned int i=0;
    for (i=0; i<=boardHashMap->curNumberOfEntries; i++)
    {
      char * key = hashMap_GetKeyAtIndex(boardHashMap,i);

      if (key!=0)
      {
       strcat(rqst->content," <a href=\"threadIndexView.html?board=");
       strcat(rqst->content,key);
       strcat(rqst->content,"\">");
       strcat(rqst->content," Board ");
       strcat(rqst->content,key);
       strcat(rqst->content," </a> <br>");
      }

    }

    strcat(rqst->content,"</body></html>");

   rqst->contentSize=strlen(rqst->content);
}
