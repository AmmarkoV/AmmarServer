#include "recvHTTPHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
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

 unsigned int result=1 , doneReceiving=0;
 int opres=0;

 transaction->incomingHeader.headerRAWSize = 0 ;
 transaction->incomingHeader.MAXheaderRAWSize = MAX_HTTP_REQUEST_HEADER+1 ;
 char  * incomingRequest = (char*)  malloc(sizeof(char) * (transaction->incomingHeader.MAXheaderRAWSize+5) );
 if (incomingRequest==0) { error("Could not allocate enough memory for Header "); return 0; }
 memset(incomingRequest,0,sizeof(char) * (transaction->incomingHeader.MAXheaderRAWSize+5) );

 transaction->incomingHeader.headerRAW = incomingRequest ;
 transaction->incomingHeader.headerRAW[0]=0;


 fprintf(stderr,"KeepAlive Server Loop , Waiting for a valid HTTP header..\n");
 while (
        (!doneReceiving) &&
        (!transaction->incomingHeader.failed) &&
        (instance->server_running)
       )
 {
  ++instance->statistics.recvOperationsStarted;
   opres=ASRV_Recv(
              instance ,
              transaction,
              &transaction->incomingHeader.headerRAW[transaction->incomingHeader.headerRAWSize],
              (unsigned int) transaction->incomingHeader.MAXheaderRAWSize - transaction->incomingHeader.headerRAWSize ,
              0
             );
   // More input => Null Terminate , we have allocated extra space so this is always ok..!
   if (opres>0) { transaction->incomingHeader.headerRAW[transaction->incomingHeader.headerRAWSize + opres]=0; }
  ++instance->statistics.recvOperationsFinished;

  //Error Receiving..
  if (opres<0)
   {
     if ( (errno==EWOULDBLOCK) && (transaction->incomingHeader.headerRAWSize==0) )
     {
       //fprintf(stderr,"KeepAlive Connection Timed Out\n");
       transaction->clientDisconnected=1;
     } else
     {
      fprintf(stderr,"Error receiving (%u bytes recvd) ..\n",transaction->incomingHeader.headerRAWSize);
      printRecvError();
     }

     result=0;
     break;
   }
  else
  //Client shutdown connection..
  if (opres==0) { fprintf(stderr,"Client shutdown while receiving HTTP Header..\n"); result=0; break; }
  else
  //Received new data
   {
      //Count incoming data
      if (opres>0)
       { instance->statistics.totalDownloadKB+=(unsigned long) opres/1024; }
      transaction->incomingHeader.headerRAWSize+=opres;
      fprintf(stderr,"Got %d more bytes ( %u total / %u max )\n",opres,transaction->incomingHeader.headerRAWSize,transaction->incomingHeader.MAXheaderRAWSize);

      if (!keepAnalyzingHTTPHeader(instance,transaction))
      {
        fprintf(stderr,"We are done receiving and analyzing this HTTPHeader\n");
        result=1;
        doneReceiving=1;
        break;
      }
    } // --  --  --  --  --

    //Check if what we have received is all that there is
    if ( HTTPRequestIsComplete(instance,transaction) )
    {
       result=1;
       doneReceiving=1;
       break;
    } else
     if (
          //We filled our headerRAW buffer
          (transaction->incomingHeader.headerRAWSize >= transaction->incomingHeader.MAXheaderRAWSize) &&
          //And we are talking about a POST request
          (transaction->incomingHeader.requestType == POST ) &&
          //And POST is enabled both by configuration and this instance
          ((instance->settings.ENABLE_POST)&&(MASTER_ENABLE_POST))
         )
        {
           //Try to grow our header , If we can't then stop
           if (!growHeader(transaction))
           {
                fprintf(stderr,"Failed to grow the header :( \n");
                result=0;
                doneReceiving=1;
                break;
           }
        }
  } // END OF RECEIVE LOOP

  //Less spam
  //fprintf(stderr,"Finished receiving and parsing HTTP header..\n");

  if (
       (!transaction->incomingHeader.failed) &&
       (transaction->incomingHeader.requestType == POST ) &&
       ((instance->settings.ENABLE_POST)&&(MASTER_ENABLE_POST))
      )
  {
    transaction->incomingHeader.POSTrequestBody = transaction->incomingHeader.headerRAW+transaction->incomingHeader.headerRAWHeadSize+1;
    transaction->incomingHeader.POSTrequestBodySize = transaction->incomingHeader.headerRAWSize-transaction->incomingHeader.headerRAWHeadSize;
  }


  if (transaction->incomingHeader.failed)
   {
     fprintf(stderr,"Failed receiving header , marking it ..\n");
     result = 0;
   }

  if (!result)
  {
    //Remove incoming request here
    if (transaction->incomingHeader.headerRAW!=0)
         { free(transaction->incomingHeader.headerRAW); }

    transaction->incomingHeader.headerRAW = 0;
    transaction->incomingHeader.headerRAWSize = 0;
    //
    transaction->incomingHeader.POSTrequestBody = 0;
    transaction->incomingHeader.POSTrequestBodySize = 0;
    transaction->incomingHeader.POSTrequest = 0;
    transaction->incomingHeader.POSTrequestSize = 0;
  }

 return result;
}

