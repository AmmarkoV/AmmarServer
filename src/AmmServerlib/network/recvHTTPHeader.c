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
 transaction->incomingHeader.headerRAW = incomingRequest ;
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
  if (opres<0)  { printRecvError(); result=0; break; }
  else
  //Client shutdown connection..
  if (opres==0) { fprintf(stderr,"Client shutdown while receiving HTTP Header..\n"); result=0; break; }
  else
  //Received new data
   {
      //Count incoming data
      instance->statistics.totalDownloadKB+=opres;
      incomingRequestLength+=opres;
      transaction->incomingHeader.headerRAWSize = incomingRequestLength;
      fprintf(stderr,"Got %d bytes ( %u total )\n",opres,incomingRequestLength);
      fprintf(stderr,"BODY : %s \n",incomingRequest);

      if (!keepAnalyzingHTTPHeader(instance,transaction))
      {
        fprintf(stderr,"We are done receiving and analyzing this HTTPHeader\n");
        result=1;
        doneReceiving=1;
        break;
      } else
      {
        //We have a chunk of input but not done yet , have we run out of space ?
        if (incomingRequestLength>=MAXincomingRequestLength)
        {
           if (!growHeader(transaction))
           {
                result=0;
                doneReceiving=1;
                break;
           }
        } else
        {
          //Just got a small chunk of input if we ended up here..
        }
      }
    } // --  --  --  --  --


    fprintf(stderr,"Checking if HTTPHeader is complete\n");
    if ( HTTPHeaderIsComplete(instance,transaction) )
    {
       fprintf(stderr,"It Is\n");
       doneReceiving=1;
       break;
    } else
    {
       fprintf(stderr,"It Is Not\n");
    }

  }

  fprintf(stderr,"Finished receiving and parsing HTTP header..\n");

  if (!result)
  {
    //Remove incoming request here
    incomingRequestLength=0;
    if (incomingRequest!=0) { free(incomingRequest); }

    transaction->incomingHeader.headerRAW = 0;
    transaction->incomingHeader.headerRAWSize = 0;
  } else
  {
    transaction->incomingHeader.headerRAW = incomingRequest;
    transaction->incomingHeader.headerRAWSize = incomingRequestLength;

  }

 return result;
}
