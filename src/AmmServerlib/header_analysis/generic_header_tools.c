#include "generic_header_tools.h"

#include "../tools/logs.h"
#include "../tools/http_tools.h"
#include "../stringscanners/postHeader.h"
#include "../stringscanners/httpHeader.h"
#include "../server_configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int recalculateHeaderFieldsBasedOnANewBaseAddress(struct HTTPTransaction * transaction)
{
  if ( (transaction==0) || (transaction->incomingHeader.headerRAW==0) )
  {
   return 0;
  }
   char * newbase=transaction->incomingHeader.headerRAW ;
   transaction->incomingHeader.cookie             = newbase+ transaction->incomingHeader.cookieIndex;
   transaction->incomingHeader.host               = newbase + transaction->incomingHeader.hostIndex;
   transaction->incomingHeader.referer            = newbase + transaction->incomingHeader.refererIndex;
   transaction->incomingHeader.eTag               = newbase + transaction->incomingHeader.eTagIndex;
   transaction->incomingHeader.userAgent          = newbase + transaction->incomingHeader.userAgentIndex;
   transaction->incomingHeader.contentType        = newbase + transaction->incomingHeader.contentTypeIndex;
   transaction->incomingHeader.contentDisposition = newbase + transaction->incomingHeader.contentDispositionIndex;
   transaction->incomingHeader.boundary           = newbase + transaction->incomingHeader.boundaryIndex;
   return 1;
}



int growHeader(struct HTTPTransaction * transaction)
{
 if (transaction==0) { return 0; }

  struct HTTPHeader * hdr =  &transaction->incomingHeader;

 //hdr->headerRAW

/*

 if (hdr->MAXheaderRAWSize < MAX_HTTP_POST_REQUEST_HEADER )
   { //There is still room for incrementing the size of the buffer
     *MaxIncomingRequestLength += HTTP_POST_GROWTH_STEP_REQUEST_HEADER;
     if (*MaxIncomingRequestLength > MAX_HTTP_POST_REQUEST_HEADER )
            { *MaxIncomingRequestLength = MAX_HTTP_POST_REQUEST_HEADER; }


     char  * largerRequest = (char * )  realloc (incomingRequest, sizeof(char) * (*MaxIncomingRequestLength+2) );

     if ( largerRequest!=0 )
        {
         fprintf(stderr,"Successfully grown input header using %u bytes\n",*MaxIncomingRequestLength);
         incomingRequest=largerRequest;
        } else
        {
          //Could not grow POST
          free(incomingRequest);
          return 0;
        }
   }*/

  return 0;
}
