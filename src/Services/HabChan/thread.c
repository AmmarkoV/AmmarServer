
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "state.h"

#include "../../AmmServerlib/AmmServerlib.h"

void * prepareThreadIndexView(struct AmmServer_DynamicRequest  * rqst)
{
  strcpy(rqst->content,"<html><body>Welcome to Hab Chan");

  if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         char * boardID = (char *) malloc ( 256 * sizeof(char) );
         if (boardID!=0)
          {
            if ( _GET(default_server,rqst,"board",boardID,256) )
             {
               strcat(rqst->content,"GOT A BOARD !!!  : "); strcat(rqst->content,boardID); strcat(rqst->content," ! ! <br>");
             }
            free(boardID);
          }
       }
    }
   strcat(rqst->content,"</body></html>" );
   rqst->contentSize=strlen(rqst->content);
}
