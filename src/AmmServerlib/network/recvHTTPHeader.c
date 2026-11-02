#include "recvHTTPHeader.h"
// --------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// --------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/uio.h>
// --------------------------------------------
#include "../AmmServerlib.h"
#include "../server_configuration.h"
// --------------------------------------------
#include "../network/networkAbstraction.h"
// --------------------------------------------
#include "../header_analysis/generic_header_tools.h"
#include "../header_analysis/http_header_analysis.h"
#include "../header_analysis/post_header_analysis.h"
#include "../header_analysis/chunked_transfer.h"
#include "../header_analysis/get_data.h"
#include "../header_analysis/post_data.h"
// --------------------------------------------
#include "file_server.h"
#include "../tools/logs.h"


#define EXTRA_SPACE 32

/*Send a final error response from inside the receive path and remember that it
  has been sent , so ServeClientKeepAliveLoop() does not emit a second 400.*/
static int AbortRequestWithError(
                                  struct AmmServer_Instance * instance,
                                  struct HTTPTransaction * transaction,
                                  unsigned int errorCode
                                )
{
  transaction->incomingHeader.keepalive=0;
  transaction->incomingHeader.responseAlreadySent=1;
  if (!SendErrorFile(instance,transaction,errorCode))
   { fprintf(stderr,"Failed sending error %u while receiving request\n",errorCode); }
  return 0;
}

/*Validate request framing as soon as the header is fully parsed , before any
  body byte is read and before a 100 Continue is emitted.  Returns 1=valid ,
  0=request aborted ( an error response has already been sent ).*/
static int ValidateParsedRequest(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
  struct HTTPHeader * output=&transaction->incomingHeader;

  if (output->headerValidated) { return 1; }
  if (output->headerRAWHeadSize==0) { return 1; } //Header is not complete yet , validate on a later call
  output->headerValidated=1;

  if ( (output->requestType==BAD) || (output->requestType==NONE) ) { return 1; } //The normal pipeline answers these

  if (output->hostHeaderCount>1)
   {
     AmmServer_Warning("Request has more than one Host header\n");
     return AbortRequestWithError(instance,transaction,400);
   }

  if ( (output->httpVersionMajor==1) && (output->httpVersionMinor>=1) && (output->hostHeaderCount==0) )
   {
     AmmServer_Warning("HTTP/1.1 request without a Host header\n");
     return AbortRequestWithError(instance,transaction,400);
   }

  if ( (output->chunked) && (output->hasContentLength) )
   {
     AmmServer_Warning("Request mixes Content-Length and Transfer-Encoding: chunked\n");
     return AbortRequestWithError(instance,transaction,400);
   }

  if (output->contentLengthInvalid)
   {
     AmmServer_Warning("Request has a duplicate or invalid Content-Length header\n");
     return AbortRequestWithError(instance,transaction,400);
   }

  if (output->transferEncodingUnsupported)
   {
     AmmServer_Warning("Unsupported Transfer-Encoding requested\n");
     return AbortRequestWithError(instance,transaction,501);
   }

  if ( (output->chunked) && (output->requestType!=POST) )
   {
     AmmServer_Warning("Chunked request body is only supported for POST\n");
     return AbortRequestWithError(instance,transaction,400);
   }

  if ( (output->requestType==POST) && (output->hasContentLength) &&
       (output->ContentLength>instance->settings.MAX_POST_TRANSACTION_SIZE) )
   {
     AmmServer_Warning("Request body exceeds the configured maximum transaction size\n");
     return AbortRequestWithError(instance,transaction,413);
   }

  if (output->expectUnsupported)
   {
     AmmServer_Warning("Unsupported Expect header\n");
     return AbortRequestWithError(instance,transaction,417);
   }

  if ( (output->expectContinue) && (output->requestType==POST) &&
       ( (!instance->settings.ENABLE_POST) || (!MASTER_ENABLE_POST) ) )
   {
     AmmServer_Warning("Expect: 100-continue on a POST request but POST handling is disabled\n");
     return AbortRequestWithError(instance,transaction,417);
   }

  if (!KEEPALIVE_ENABLED) { output->keepalive=0; }
  return 1;
}

int receiveAndParseIncomingHTTPRequest(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
 //Clear incoming header structure..!
 memset ( &transaction->incomingHeader ,0 , sizeof(struct HTTPHeader) );
  //transaction->clientSock;

 unsigned int successulHTTPTransaction=1 , doneReceiving=0;
 int chunkedBodyDecoded=0;
 int opres=0;

 transaction->incomingHeader.headerRAWSize = 0 ;
 transaction->incomingHeader.MAXheaderRAWSize = MAX_HTTP_REQUEST_HEADER;
 char  * incomingRequest = (char*)  malloc(sizeof(char) * (transaction->incomingHeader.MAXheaderRAWSize+EXTRA_SPACE) );
 if (incomingRequest==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }
 memset(incomingRequest,0,sizeof(char) * (transaction->incomingHeader.MAXheaderRAWSize+EXTRA_SPACE) );

 transaction->incomingHeader.headerRAW = incomingRequest ; //headerRAW malloc here
 transaction->incomingHeader.headerRAW[0]=0;


 //fprintf(stderr,"KeepAlive Server Loop , Waiting for a valid HTTP header..\n");
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

     successulHTTPTransaction=0;
     break;
   }
  else
  //Client shutdown connection..
  if (opres==0) { fprintf(stderr,"Client shutdown while receiving HTTP Header..\n"); successulHTTPTransaction=0; break; }
  else
  //Received new data
   {
      //Count incoming data
      if (opres>0)
       { instance->statistics.totalDownloadKB+=(unsigned long) opres/1024; }
      transaction->incomingHeader.headerRAWSize+=opres;
      //fprintf(stderr,"Got %d more bytes ( %u total / %u max )\n",opres,transaction->incomingHeader.headerRAWSize,transaction->incomingHeader.MAXheaderRAWSize);

      if (!keepAnalyzingHTTPHeader(instance,transaction))
      {
        //Was unconditional : one more write to an unbuffered stderr on the serving thread , per request.
        #if DEBUG_MESSAGES
        fprintf(stderr,"We are done receiving and analyzing this HTTPHeader\n");
        #endif // DEBUG_MESSAGES
        successulHTTPTransaction=1;
        doneReceiving=1;
        break;
      }
    } // --  --  --  --  --

    //Validate Host / framing as soon as the complete header is available
    if (!ValidateParsedRequest(instance,transaction))
    {
      successulHTTPTransaction=0;
      doneReceiving=1;
      break;
    }

    //Check if what we have received is all that there is
    int requestIsComplete=HTTPRequestIsComplete(instance,transaction);
    if ( requestIsComplete )
    {
       if ( (transaction->incomingHeader.failed) && (transaction->incomingHeader.errorResponseCode!=0) )
       {
         //HTTPRequestIsComplete() detected malformed/oversized framing and knows which status fits
         AbortRequestWithError(instance,transaction,transaction->incomingHeader.errorResponseCode);
         successulHTTPTransaction=0;
       } else
       {
         successulHTTPTransaction=1;
       }
       doneReceiving=1;
       break;
    }

    //Not complete yet : send the interim 100 Continue before blocking in recv() for the body.
    //It is sent at most once , only for a valid POST that is actually waiting for its body.
    if ( (transaction->incomingHeader.expectContinue) &&
         (!transaction->incomingHeader.expectContinueSent) &&
         (transaction->incomingHeader.headerRAWHeadSize!=0) &&
         (transaction->incomingHeader.requestType==POST) &&
         ( (transaction->incomingHeader.chunked) || (transaction->incomingHeader.ContentLength>0) ) )
     {
       const char * continueResponse="HTTP/1.1 100 Continue\r\n\r\n";
       int continueRes=ASRV_Send(
                                  instance,
                                  transaction,
                                  continueResponse,
                                  strlen(continueResponse),
                                  MSG_WAITALL|MSG_NOSIGNAL
                                );
       if (continueRes>0)
        {
          transaction->incomingHeader.expectContinueSent=1;
        } else
        {
          AmmServer_Warning("Could not send the 100 Continue interim response\n");
          successulHTTPTransaction=0;
          doneReceiving=1;
          break;
        }
     }

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
               successulHTTPTransaction=0;
               doneReceiving=1;
               break;
           }
       }
  } // END OF RECEIVE LOOP



   //DUMP THE RECEIVED HEADER IN A FILE FOR DEBUGGING PURPOSES
   //AmmServer_WriteFileFromMemory(
   //                               "RECEIVED.txt",
   //                               transaction->incomingHeader.headerRAW ,
   //                               transaction->incomingHeader.headerRAWSize
   //                             );




  ///-----------------------------------------------------------------------------------------------------
  ///-----------------------------------------------------------------------------------------------------
  ///-----------------------------------------------------------------------------------------------------
  ///JUST ENDED WITH THE SOCKET STUFF RECEIVING THE HEADER, WE NOW HAVE EVERYTHING PARSED MORE OR LESS
  ///-----------------------------------------------------------------------------------------------------
  ///-----------------------------------------------------------------------------------------------------
  //fprintf(stderr,"Finished receiving and parsing HTTP header..\n");  //Less spam
  ///-----------------------------------------------------------------------------------------------------
  ///DECHUNK THE BODY ( in place ) , THEN RE-REGISTER MULTIPART BOUNDARIES
  ///-----------------------------------------------------------------------------------------------------
  if ( (successulHTTPTransaction) && (!transaction->incomingHeader.failed) && (transaction->incomingHeader.chunked) )
  {
    struct HTTPHeader * chunkedHeader=&transaction->incomingHeader;
    unsigned int headerLength=chunkedHeader->headerRAWHeadSize+1;
    char * chunkedBody=chunkedHeader->headerRAW+headerLength;
    unsigned int chunkedBodyLength=chunkedHeader->headerRAWSize-headerLength;
    unsigned int decodedSize=0;

    enum HTTPChunkedScanResult decodeResult=HTTPChunkedBodyDecode(
                                                                   chunkedBody,
                                                                   chunkedBodyLength,
                                                                   instance->settings.MAX_POST_TRANSACTION_SIZE,
                                                                   &decodedSize
                                                                 );

    if (decodeResult!=HTTP_CHUNKED_COMPLETE)
    {
      AbortRequestWithError(
                             instance,
                             transaction,
                             (decodeResult==HTTP_CHUNKED_ERROR_TOO_BIG) ? 413 : 400
                           );
      successulHTTPTransaction=0;
    } else
    {
      //Shrink the request buffer to header + decoded payload , so every later scan is bounded correctly
      chunkedHeader->headerRAWSize=headerLength+decodedSize;
      chunkedHeader->headerRAW[chunkedHeader->headerRAWSize]=0;
      chunkedHeader->POSTrequestBody=chunkedBody;
      chunkedHeader->POSTrequestBodySize=decodedSize;
      chunkedHeader->ContentLength=decodedSize;
      chunkedHeader->hasContentLength=1;
      chunkedHeader->chunked=0;
      chunkedHeader->parsingStartOffset=headerLength;
      chunkedBodyDecoded=1;

      //The incremental parser never saw the chunked body lines ( see CapRequestLengthToHeaderEnd ) ,
      //so multipart boundaries have to be re-discovered from the decoded buffer now.
      if ( (chunkedHeader->boundary!=0) &&
           (instance->settings.ENABLE_POST) && (MASTER_ENABLE_POST) )
      {
        wipePOSTData(chunkedHeader);
        AnalyzePOSTBodyLines(
                              instance,
                              chunkedHeader,
                              chunkedBody,
                              decodedSize,
                              chunkedHeader->parsingCurrentLine+1
                            );
      }
    }
  }

  struct HTTPHeader * output  = &transaction->incomingHeader;

  if ( (successulHTTPTransaction) && (!finalizeGETData(output)) )
  {
      if (output->GETrequest==0)
      {
        if ( (output->headerRAW==0) || (output->headerRAWSize==0) )
            {
               AmmServer_Error("Received header was empty, this means no GET request");
            } else
            {
               //fprintf(stderr,"Request `%s` did not have arguments\n",output->headerRAW);
               //No arguments, no problem
               //fprintf(stderr,"Request did not have arguments\n");
            }
      } else
      {
        AmmServer_Error("finalizeGETData: Server failed to parse GET data");
      }
  }

  ///IN CASE WE ARE USING POST REQUESTS WE WILL JUST NEED TO FINALIZE THE DATA PARSED THROUGH THE FILE TO SETUP BOUNDARIES ETC..
  if (
       (successulHTTPTransaction) &&
       (!transaction->incomingHeader.failed) &&
       (transaction->incomingHeader.requestType == POST ) &&
       ((instance->settings.ENABLE_POST)&&(MASTER_ENABLE_POST))
      )
  {
    //We need to count our POST header request sizes ( a dechunked body already has them )
    if (!chunkedBodyDecoded)
    {
      transaction->incomingHeader.POSTrequestBody = transaction->incomingHeader.headerRAW+transaction->incomingHeader.headerRAWHeadSize+1;
      transaction->incomingHeader.POSTrequestBodySize = transaction->incomingHeader.headerRAWSize-transaction->incomingHeader.headerRAWHeadSize;
    }


    //AmmServer_WriteFileFromMemory(
    //                               "POST.txt",
    //                               transaction->incomingHeader.POSTrequestBody ,
    //                               transaction->incomingHeader.POSTrequestBodySize
    //                             );

    //We need to finalize the POST data processing
    if (!finalizePOSTData(output))
    {
      AmmServer_Error("Server failed to parse POST data");
    }


    //AmmServer_WriteFileFromMemory(
    //                               "POSTFINALIZED.txt",
    //                               transaction->incomingHeader.POSTrequestBody ,
    //                               transaction->incomingHeader.POSTrequestBodySize
    //                             );



  }

  //--------------------- --------------------- ---------------------
  if (transaction->incomingHeader.failed)
   {
     fprintf(stderr,"Failed receiving header , marking it ..\n");
     successulHTTPTransaction = 0;
   }
  //--------------------- --------------------- ---------------------

  ///--------------------- --------------------- ---------------------
  /// IN CASE WE FAIL WE NEED TO CLEAN UP EVERYTHING FOR OUR CLIENT
  ///--------------------- --------------------- ---------------------
  if (!successulHTTPTransaction)
  {
    //Remove incoming request here
    if (transaction->incomingHeader.headerRAW!=0)
         {
          safeFree(
                    transaction->incomingHeader.headerRAW ,
                    transaction->incomingHeader.headerRAWSize
                  );
         }

    transaction->incomingHeader.headerRAW = 0;
    transaction->incomingHeader.headerRAWSize = 0;
    //
    transaction->incomingHeader.POSTrequestBody = 0;
    transaction->incomingHeader.POSTrequestBodySize = 0;
    transaction->incomingHeader.POSTrequest = 0;
    transaction->incomingHeader.POSTrequestSize = 0;

    //Every one of these points inside headerRAW , which is about to be freed. Logging and the
    //caller's error path still read some of them , so null them before the free to avoid a UAF.
    transaction->incomingHeader.GETrequest = 0;
    transaction->incomingHeader.GETrequestSize = 0;
    transaction->incomingHeader.cookie = 0;
    transaction->incomingHeader.cookieLength = 0;
    transaction->incomingHeader.host = 0;
    transaction->incomingHeader.hostLength = 0;
    transaction->incomingHeader.referer = 0;
    transaction->incomingHeader.refererLength = 0;
    transaction->incomingHeader.eTag = 0;
    transaction->incomingHeader.eTagLength = 0;
    transaction->incomingHeader.userAgent = 0;
    transaction->incomingHeader.userAgentLength = 0;
    transaction->incomingHeader.contentType = 0;
    transaction->incomingHeader.contentTypeLength = 0;
    transaction->incomingHeader.contentDisposition = 0;
    transaction->incomingHeader.contentDispositionLength = 0;
    transaction->incomingHeader.boundary = 0;
    transaction->incomingHeader.boundaryLength = 0;

    struct HTTPHeader * output  = &transaction->incomingHeader;
    wipeGETData(output);
    wipePOSTData(output);
    transaction->incomingHeader.POSTItemNumber = 0;
  }
  ///--------------------- --------------------- ---------------------

 return successulHTTPTransaction;
}

