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
 transaction->incomingHeader.headerRAWSize = 0 ;
 transaction->incomingHeader.MAXheaderRAWSize = MAX_HTTP_REQUEST_HEADER+1 ;
 char  * incomingRequest = (char*)  malloc(sizeof(char) * (transaction->incomingHeader.MAXheaderRAWSize+5) );
 transaction->incomingHeader.headerRAW = incomingRequest ;
 if (incomingRequest==0) { error("Could not allocate enough memory for Header "); return 0; }
 transaction->incomingHeader.headerRAW[0]=0;


 fprintf(stderr,"KeepAlive Server Loop , Waiting for a valid HTTP header..\n");
 while (
        (!doneReceiving) &&
        (instance->server_running)
       )
 {
  ++instance->statistics.recvOperationsStarted;
   opres=recv(
              transaction->clientSock,
              &transaction->incomingHeader.headerRAW[transaction->incomingHeader.headerRAWSize],
              (unsigned int) transaction->incomingHeader.MAXheaderRAWSize - transaction->incomingHeader.headerRAWSize ,
              0
             );
   // More input => Null Terminate , we have allocated extra space so this is always ok..!
   if (opres>0) { transaction->incomingHeader.headerRAW[transaction->incomingHeader.headerRAWSize + opres]=0; }
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
      transaction->incomingHeader.headerRAWSize+=opres;
      fprintf(stderr,"Got %d bytes ( %u total / %u max )\n",opres,transaction->incomingHeader.headerRAWSize,transaction->incomingHeader.MAXheaderRAWSize);
      fprintf(stderr,"BODY : %s \n",transaction->incomingHeader.headerRAW);

      if (!keepAnalyzingHTTPHeader(instance,transaction))
      {
        fprintf(stderr,"We are done receiving and analyzing this HTTPHeader\n");
        result=1;
        doneReceiving=1;
        break;
      }
    } // --  --  --  --  --


    if ( HTTPHeaderIsComplete(instance,transaction) )
    {
       result=1;
       doneReceiving=1;
       break;
    } else
     if (
             (transaction->incomingHeader.headerRAWSize>=transaction->incomingHeader.MAXheaderRAWSize) &&
             (transaction->incomingHeader.requestType == POST ) &&
             (ENABLE_POST)
           )
        {
           if (!growHeader(transaction))
           {
                fprintf(stderr,"Failed to grow the header :( \n");
                result=0;
                doneReceiving=1;
                break;
           }
        }


  }

  fprintf(stderr,"Finished receiving and parsing HTTP header..\n");

  if (!result)
  {
    //Remove incoming request here
    if (transaction->incomingHeader.headerRAW!=0)
         { free(transaction->incomingHeader.headerRAW); }

    transaction->incomingHeader.headerRAW = 0;
    transaction->incomingHeader.headerRAWSize = 0;
  }

 return result;
}
