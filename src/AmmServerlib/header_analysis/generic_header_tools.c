#include "generic_header_tools.h"

#include "../tools/logs.h"
#include "../tools/http_tools.h"
#include "../stringscanners/postHeader.h"
#include "../stringscanners/httpHeader.h"
#include "../server_configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int clearHeader(struct HTTPHeader * hdr)
{
 //Clear incoming header structure..!
  memset ( hdr ,0 , sizeof(struct HTTPHeader) );
 return 1;
}


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

 unsigned wannabeHeaderSize = hdr->MAXheaderRAWSize + HTTP_POST_GROWTH_STEP_REQUEST_HEADER;
 if (hdr->MAXheaderRAWSize < MAX_HTTP_POST_REQUEST_HEADER )
   { //There is still room for incrementing the size of the buffer
     if (wannabeHeaderSize > MAX_HTTP_POST_REQUEST_HEADER )
            { wannabeHeaderSize = MAX_HTTP_POST_REQUEST_HEADER; }

     char  * newBuffer = (char * )  realloc (hdr->headerRAW , sizeof(char) * (hdr->MAXheaderRAWSize+1) );

     if (newBuffer!=0 )
     {
       hdr->headerRAW=newBuffer;
       hdr->MAXheaderRAWSize = wannabeHeaderSize;
       recalculateHeaderFieldsBasedOnANewBaseAddress(transaction);
       return 1;
     } else
     {
       fprintf(stderr,"Failed to grow header , out of memory ? \n");
     }
    } else
    {
      fprintf(stderr,"Failed to grow header , reached max header limit \n");
    }


  return 0;
}
