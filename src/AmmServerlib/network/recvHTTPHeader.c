#include "recvHTTPHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>

#include "../AmmServerlib.h"
#include "../server_configuration.h"

#include "../header_analysis/generic_header_tools.h"
#include "../header_analysis/http_header_analysis.h"
#include "../header_analysis/post_header_analysis.h"





int handleHeaderGrowingHere(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
   //!TODO : Also update the parsing point if realloc
   char * parsingStartPoint;
   unsigned int parsingStartingLine;






    recalculateHeaderFieldsBasedOnANewBaseAddress(transaction);

    /*
         //We filled our buffer , if we have a POST request there is a different limit
         //so that we can upload files
         if ( ( HTTPHeaderIsPOST(incomingRequest,incomingRequestLength ) ) && ( ENABLE_POST ) )
          {
            incomingRequest = growPOSTHeader(&MAXincomingRequestLength,incomingRequest);
            if (incomingRequest==0)
            {
             fprintf(stderr,"Could not grow POST header, dropping client \n");
             result=0;
             break;
            }
          } else
          {
            fprintf(stderr,"The request would overflow , dropping client \n");
            result=0;
            break;
          }
*/
 return 1;
}


int receiveAndParseIncomingHTTPRequest(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
 //Clear incoming header structure..!
 memset ( &transaction->incomingHeader ,0 , sizeof(struct HTTPHeader) );
  //transaction->clientSock;

 int opres=0;

 unsigned int result=1;
 unsigned int doneReceiving=0;
 unsigned int incomingRequestLength = 0 ;
 unsigned int MAXincomingRequestLength = MAX_HTTP_REQUEST_HEADER+1 ;
 char  * incomingRequest = (char*)  malloc(sizeof(char) * (MAXincomingRequestLength+2) );
 if (incomingRequest==0) { error("Could not allocate enough memory for Header "); return 0; }
 incomingRequest[0]=0;


 fprintf(stderr,"KeepAlive Server Loop , Waiting for a valid HTTP header..\n");
 while (
        (!doneReceiving) &&
        (instance->server_running)
       )
 {
  ++instance->statistics.recvOperationsStarted;
   opres=recv(transaction->clientSock,&incomingRequest[incomingRequestLength],MAXincomingRequestLength-incomingRequestLength,0);
  ++instance->statistics.recvOperationsFinished;

  //Error Receiving..
  if (opres<0)  { printRecvError(); result=0; break; } else
  //Client shutdown connection..
  if (opres==0) { fprintf(stderr,"client shutdown ..\n"); result=0; break; } else
  //Received new data
   {
      //Count incoming data
      instance->statistics.totalDownloadKB+=opres;
      incomingRequestLength+=opres;
      fprintf(stderr,"Got %d bytes ( %u total )\n",opres,incomingRequestLength);

      //!TODO : also accommodate parsing progress inside transaction
      if (!keepAnalyzingHTTPHeader(instance,transaction))
      {
        fprintf(stderr,"We are done receiving and analyzing this HTTPHeader\n");
        result=1;
        break;
      } else
      {
        //We have a chunk of input but not done yet , have we run out of space ?
        if (incomingRequestLength>=MAXincomingRequestLength)
        {
         handleHeaderGrowingHere(instance,transaction);
        } else
        {
          //Just got a small chunk of input if we ended up here..
        }
      }
    } // --  --  --  --  --
  }

  fprintf(stderr,"Finished Waiting for a valid HTTP header..\n");

  if (!result)
  {
    //Remove incoming request here
    incomingRequestLength=0;
    if (incomingRequest!=0) { free(incomingRequest); }
  } else
  {
    transaction->incomingHeader.headerRAW = incomingRequest;
    transaction->incomingHeader.headerRAWSize = incomingRequestLength;

  }

 return result;
}
