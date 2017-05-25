#include "generic_header_tools.h"
#include "http_header_analysis.h"

#include "../tools/logs.h"
#include "../tools/http_tools.h"
#include "../stringscanners/postHeader.h"
#include "../stringscanners/httpHeader.h"
#include "../server_configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CR 13
#define LF 10



int HTTPHeaderIsHEAD(char * request , unsigned int requestLength)
{
  if (requestLength<4) { return 0; }
  if ((request[0]=='H')&&(request[1]=='E')&&(request[2]=='A')&&(request[3]=='D'))
  {
    return 1;
  }
  return 0;
}

int HTTPHeaderIsPOST(char * request , unsigned int requestLength)
{
  if (requestLength<4) { return 0; }
  if ((request[0]=='P')&&(request[1]=='O')&&(request[2]=='S')&&(request[3]=='T'))
  {
    return 1;
  }
  return 0;
}

int HTTPHeaderIsGET(char * request , unsigned int requestLength)
{
  if (requestLength<3) { return 0; }
  if ((request[0]=='G')&&(request[1]=='E')&&(request[2]=='T'))
  {
    return 1;
  }
  return 0;
}

void printNBytes(char * buf,unsigned int n)
{
  unsigned int i=0;
  for (i=0; i<n; i++)
  {
      fprintf(stderr,"%c",buf[i]);
  }
}





int HTTPHeaderScanForHeaderEndFromStart(char * request,unsigned int request_length,unsigned int *thisScanResult)
{
  /*  This call returns 1 when we find two subsequent newline characters
      which mark the ending of an HTTP header..! The function returns 1 or 0 ..! */
  if (request_length<4) {  fprintf(stderr,"Header too small ( %u ) to check for an ending..!\n",request_length); return 0; } // at least LF LF is expected :P

  fprintf(stderr,"Checking if request with %u chars is complete .. ",request_length);
  unsigned int i=3;
  while (i<request_length)
   {
      if ( request[i]==LF )
       {
        if (i>=1) {
                    if (( request[i-1]==LF )&&( request[i]==LF ))
                     {
                      fprintf(stderr,"it is ( ux @%u [ %u %u ] ) \n",i,request[i-1],request[i]);
                      *thisScanResult=i;
                      return i;
                     }
                  } /* unix 2x new line sequence */

        if (i>=3) {
                    if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF ))
                    {
                     fprintf(stderr,"it is ( win @%u [ %u %u %u %u ] )\n",i,request[i-3],request[i-2],request[i-1],request[i]);
                     *thisScanResult=i;
                     return i;
                    }
                  } /* windows 2x new line sequence */
       }
     ++i;
   }

   fprintf(stderr,"it isn't \n");
   return 0;
}





int HTTPHeaderScanForHeaderEndFromEnd(char * request,unsigned int dontSearchBefore,unsigned int request_length,unsigned int *thisScanResult)
{
  /*  This call returns 1 when we find two subsequent newline characters
      which mark the ending of an HTTP header..! The function returns 1 or 0 ..! */
  if (request_length<4) {  fprintf(stderr,"Header too small ( %u ) to check for an ending..!\n",request_length); return 0; } // at least LF LF is expected :P

  fprintf(stderr,"Checking if request with %u chars is complete .. ",request_length);
  unsigned int i=request_length-1;
  while (i>dontSearchBefore+1)
   {
      if ( request[i]==LF )
       {
        if (i>=1) {
                    if (( request[i-1]==LF )&&( request[i]==LF ))
                     {
                      fprintf(stderr,"it is ( ux @%u [ %u %u ] ) \n",i,request[i-1],request[i]);
                      *thisScanResult=i;
                      return i;
                     }
                  } /* unix 2x new line sequence */

        if (i>=3) {
                    if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF ))
                    {
                     fprintf(stderr,"it is ( win @%u [ %u %u %u %u ] )\n",i,request[i-3],request[i-2],request[i-1],request[i]);
                     *thisScanResult=i;
                     return i;
                    }
                  } /* windows 2x new line sequence */
       }
     --i;
   }

   fprintf(stderr,"it isn't \n");
   return 0;
}


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
  if (transaction->incomingHeader.failed) { AmmServer_Error("Will not try to grow a failed Header \n"); return 0; }

  struct HTTPHeader * hdr =  &transaction->incomingHeader;
  if (hdr == 0 )  { AmmServer_Error("Cannot grow header on transaction with no header.."); return 0; }

  struct AmmServer_Instance * instance = transaction->instance;
  if (instance == 0 )  { AmmServer_Error("Cannot grow header on transaction with no registered server instance..");  return 0; }

  fprintf(stderr,"growHeader called can grow up to %u \n",instance->settings.MAX_POST_TRANSACTION_SIZE);

  //We have a requested header Size
  unsigned int wannabeHeaderSize = hdr->MAXheaderRAWSize + HTTP_POST_GROWTH_STEP_REQUEST_HEADER;
  if (hdr->headerRAWRequestedSize!=0)
  {
   if (hdr->headerRAWRequestedSize <= instance->settings.MAX_POST_TRANSACTION_SIZE)
   {
    wannabeHeaderSize = hdr->headerRAWRequestedSize ; // + transaction->incomingHeader.headerHeadSize;
   } else
   {
    fprintf(stderr,"Cannot grow header , requested size ( %u is more than our limit %u )\n",hdr->headerRAWRequestedSize,instance->settings.MAX_POST_TRANSACTION_SIZE);
    hdr->dumpedToFile=1; fprintf(stderr,"This should be handled by dumping to /tmp/files \n");
    return 0;
   }
  }

 if (hdr->MAXheaderRAWSize < instance->settings.MAX_POST_TRANSACTION_SIZE )
   { //There is still room for incrementing the size of the buffer
     if (wannabeHeaderSize > instance->settings.MAX_POST_TRANSACTION_SIZE )
            { wannabeHeaderSize = instance->settings.MAX_POST_TRANSACTION_SIZE; }
      fprintf(stderr,"Growing ");
     char  * newBuffer = (char * )  realloc (hdr->headerRAW , sizeof(char) * (wannabeHeaderSize+2) );

     if (newBuffer!=0 )
     {
       hdr->headerRAW=newBuffer;
       hdr->MAXheaderRAWSize = wannabeHeaderSize;
       recalculateHeaderFieldsBasedOnANewBaseAddress(transaction);
       return 1;
     } else
     {
       fprintf(stderr,"Failed to grow header , out of memory ? \n");
       hdr->dumpedToFile=1; fprintf(stderr,"This should be handled by dumping to /tmp/files \n");
     }
    } else
    {
      fprintf(stderr,"Failed to grow header , reached max header limit \n");
      hdr->dumpedToFile=1; fprintf(stderr,"This should be handled by dumping to /tmp/files \n");
    }


  return 0;
}





int keepAnalyzingHTTPHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
  char * webserver_root = instance->webserver_root;
  struct HTTPHeader * output  = &transaction->incomingHeader;


  char * request = output->headerRAW + output->parsingStartOffset;
  unsigned int request_length = transaction->incomingHeader.headerRAWSize;
  char * startOfNewLine=request;

  unsigned int newLineLength=0;

  unsigned int i=0;
  while  (
            (i<request_length) &&
            (i<MAX_HTTP_REQUEST_HEADER_LINES)
         )
   {
     switch (request[i])
     {
        //If we reached a CR or LF character we might found a new line!
        case CR :
        case LF :
                  if (newLineLength>0)
                  {
                    //We've reached keepAnalyzingHTTPHeadera new line! , lets process the previous one
                    ++output->parsingCurrentLine;
                    AnalyzeHTTPLineRequest(instance,output,startOfNewLine,newLineLength,output->parsingCurrentLine,webserver_root);
                    output->parsingStartOffset+=newLineLength; //Remember where we are
                    newLineLength=0;

                    startOfNewLine = request+i+1; //+1 gets past current CR or LF
                    switch (*startOfNewLine)
                      { //Some hosts transmit CR LF so lets test for a second character
                        case CR :
                        case LF : ++startOfNewLine; ++i; break;
                      };
                    break;
                  }
        default :
                  ++newLineLength;
                 break;
     };

     ++i;
   }


  return 1;
}

/*
   The purpose of this function is dual , to findout if HTTPReceive can
*/
int HTTPRequestIsComplete(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
  unsigned int HeaderEndDetectedByCurrentScan=0;
  if (transaction->incomingHeader.requestType == POST)
  {
     //POST Requests have a Content-Length , and we detect their end using that..
       HTTPHeaderScanForHeaderEndFromStart(
                                           transaction->incomingHeader.headerRAW ,
                                           transaction->incomingHeader.headerRAWSize ,
                                           &transaction->incomingHeader.headerRAWHeadSize
                                          );
     unsigned int totalHTTPRecvSize = transaction->incomingHeader.ContentLength + transaction->incomingHeader.headerRAWHeadSize;

     fprintf(stderr,"Our header length is %lu , we got %u bytes \n" , transaction->incomingHeader.ContentLength , transaction->incomingHeader.headerRAWSize );
     if (transaction->incomingHeader.ContentLength> instance->settings.MAX_POST_TRANSACTION_SIZE)
     {
       fprintf(stderr,"Requested POST Size is too big calling it a day if we got the initial..!");
       transaction->incomingHeader.failed=1;
       return 1;
     } else
     if (totalHTTPRecvSize >  transaction->incomingHeader.headerRAWSize )
     {
       AmmServer_Warning("Header needs more space ( Recvd = %u Bytes / Header size = %u Bytes / MAX allowed size = %u Bytes )..!",totalHTTPRecvSize,transaction->incomingHeader.headerRAWHeadSize , transaction->incomingHeader.MAXheaderRAWSize);

       if ( (!instance->settings.ENABLE_POST)||(!MASTER_ENABLE_POST) )
          { AmmServer_Warning("POST functionality is not enabled in this build , so allocated space for header ( MAX_HTTP_REQUEST_HEADER )  is small , this explains what happened..!\n"); } else
          { AmmServer_Warning("You might consider tuning your AMMSET_MAX_POST_TRANSACTION_SIZE parameter to increase it ..!\n"); }


       transaction->incomingHeader.headerRAWRequestedSize = transaction->incomingHeader.ContentLength + transaction->incomingHeader.headerRAWHeadSize;
       return 0;
     } else
     if (transaction->incomingHeader.headerRAWSize <=  totalHTTPRecvSize )
     {
       return 1;
     }
  }


  //In other cases just scan for the two consecutive new lines
  return HTTPHeaderScanForHeaderEndFromEnd(
                                           transaction->incomingHeader.headerRAW ,
                                           0,
                                           transaction->incomingHeader.headerRAWSize ,
                                           &HeaderEndDetectedByCurrentScan
                                          );
}
