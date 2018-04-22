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

/*
void printNBytes(char * buf,unsigned int n)
{
  unsigned int i=0;
  for (i=0; i<n; i++)
  {
      fprintf(stderr,"%c",buf[i]);
  }
}*/





int HTTPHeaderScanForHeaderEndFromStart(char * request,unsigned int request_length,unsigned int *thisScanResult)
{
  /*  This call returns 1 when we find two subsequent newline characters
      which mark the ending of an HTTP header..! The function returns 1 or 0 ..! */
  if (request_length<4) { warningID(ASV_WARNING_BUFFER_UNDERFLOW); return 0; } // at least LF LF is expected :P

  fprintf(stderr,"HTTPHeaderScanForHeaderEndFromStart: Checking if request with %u chars is complete .. ",request_length);
  unsigned int i=3;
  while (i<request_length)
   {
      if ( request[i]==LF )
       {
        if (i>=1) {
                    if (( request[i-1]==LF )&&( request[i]==LF ))
                     {
                      fprintf(stderr,"it is ( ux @%u [ %d %d ] ) \n",i,request[i-1],request[i]);
                      *thisScanResult=i;
                      return i;
                     }
                  } /* unix 2x new line sequence */

        if (i>=3) {
                    if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF ))
                    {
                     fprintf(stderr,"it is ( win @%u [ %d %d %d %d ] )\n",i,request[i-3],request[i-2],request[i-1],request[i]);
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

  //fprintf(stderr,"HTTPHeaderScanForHeaderEndFromEnd: Checking if request with %u chars is complete .. ",request_length);
  unsigned int i=request_length-1;
  while (i>dontSearchBefore+1)
   {
      if ( request[i]==LF )
       {
        if (i>=1) {
                    if (( request[i-1]==LF )&&( request[i]==LF ))
                     {
                      //fprintf(stderr,"it is ( ux @%u [ %u %u ] ) \n",i,request[i-1],request[i]);
                      *thisScanResult=i;
                      return i;
                     }
                  } /* unix 2x new line sequence */

        if (i>=3) {
                    if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF ))
                    {
                     //fprintf(stderr,"it is ( win @%u [ %u %u %u %u ] )\n",i,request[i-3],request[i-2],request[i-1],request[i]);
                     *thisScanResult=i;
                     return i;
                    }
                  } /* windows 2x new line sequence */
       }
     --i;
   }

   //fprintf(stderr,"it isn't \n");
   return 0;
}


int clearHeader(struct HTTPHeader * hdr)
{
 //Clear incoming header structure..!
  memset ( hdr ,0 , sizeof(struct HTTPHeader) );
 return 1;
}

unsigned int _calculateRemainingDataLength(char * baseAddress,unsigned int baseAddressMemoryLength , char * whereWeAre)
{
  char * endAddress = baseAddress + baseAddressMemoryLength;
  if (whereWeAre >= endAddress)
  {
    AmmServer_Error("Something is wrong with addresses handed to _calculateRemainingDataLength, we are pointing %u bytes outside allocated memory..",(unsigned int ) (whereWeAre-endAddress));
    return 0;
  }

  return (unsigned int ) (endAddress-whereWeAre);
}



char * _recalculatePosition(const char * oldPosition,const char * oldBaseAddress,char *newAddress)
{
 if (oldPosition==0) { return 0; }

 return newAddress + (unsigned int) (oldPosition - oldBaseAddress);
}


int recalculateHeaderFieldsBasedOnANewBaseAddress(
                                                  char * oldBaseAddress,
                                                  struct HTTPTransaction * transaction
                                                  )
{
  fprintf(stderr,"recalculateHeaderFieldsBasedOnANewBaseAddress\n");

  if ( (transaction==0) || (transaction->incomingHeader.headerRAW==0) )
  {
   return 0;
  }

   char * newBaseAddress=transaction->incomingHeader.headerRAW ;
   transaction->incomingHeader.cookie             =  _recalculatePosition(transaction->incomingHeader.cookie            ,oldBaseAddress ,newBaseAddress);
   transaction->incomingHeader.host               =  _recalculatePosition(transaction->incomingHeader.host              ,oldBaseAddress ,newBaseAddress);
   transaction->incomingHeader.referer            =  _recalculatePosition(transaction->incomingHeader.referer           ,oldBaseAddress ,newBaseAddress);
   transaction->incomingHeader.eTag               =  _recalculatePosition(transaction->incomingHeader.eTag              ,oldBaseAddress ,newBaseAddress);
   transaction->incomingHeader.userAgent          =  _recalculatePosition(transaction->incomingHeader.userAgent         ,oldBaseAddress ,newBaseAddress);
   transaction->incomingHeader.contentType        =  _recalculatePosition(transaction->incomingHeader.contentType       ,oldBaseAddress ,newBaseAddress);
   transaction->incomingHeader.contentDisposition =  _recalculatePosition(transaction->incomingHeader.contentDisposition,oldBaseAddress ,newBaseAddress);
   transaction->incomingHeader.boundary           =  _recalculatePosition(transaction->incomingHeader.boundary          ,oldBaseAddress ,newBaseAddress);


   unsigned int i=0;
   for (i=0; i<transaction->incomingHeader.POSTItemNumber; i++)
   {
    if (transaction->incomingHeader.POSTItem[i].reallocateOnHeaderRAWResize)
    {
    transaction->incomingHeader.POSTItem[i].pointerStart       =  _recalculatePosition(transaction->incomingHeader.POSTItem[i].pointerStart       ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.POSTItem[i].pointerEnd         =  _recalculatePosition(transaction->incomingHeader.POSTItem[i].pointerEnd         ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.POSTItem[i].name               =  _recalculatePosition(transaction->incomingHeader.POSTItem[i].name               ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.POSTItem[i].value              =  _recalculatePosition(transaction->incomingHeader.POSTItem[i].value              ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.POSTItem[i].filename           =  _recalculatePosition(transaction->incomingHeader.POSTItem[i].filename           ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.POSTItem[i].contentDisposition =  _recalculatePosition(transaction->incomingHeader.POSTItem[i].contentDisposition ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.POSTItem[i].contentType        =  _recalculatePosition(transaction->incomingHeader.POSTItem[i].contentType        ,oldBaseAddress ,newBaseAddress);
    }
   }

   for (i=0; i<transaction->incomingHeader.GETItemNumber; i++)
   {
    if (transaction->incomingHeader.GETItem[i].reallocateOnHeaderRAWResize)
    {
    transaction->incomingHeader.GETItem[i].name       =  _recalculatePosition(transaction->incomingHeader.GETItem[i].name       ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.GETItem[i].value      =  _recalculatePosition(transaction->incomingHeader.GETItem[i].value      ,oldBaseAddress ,newBaseAddress);
    }
   }

   for (i=0; i<transaction->incomingHeader.COOKIEItemNumber; i++)
   {
    if (transaction->incomingHeader.COOKIEItem[i].reallocateOnHeaderRAWResize)
    {
    transaction->incomingHeader.COOKIEItem[i].name    =  _recalculatePosition(transaction->incomingHeader.COOKIEItem[i].name       ,oldBaseAddress ,newBaseAddress);
    transaction->incomingHeader.COOKIEItem[i].value   =  _recalculatePosition(transaction->incomingHeader.COOKIEItem[i].value      ,oldBaseAddress ,newBaseAddress);
    }
   }
   return 1;
}



int growHeader(struct HTTPTransaction * transaction)
{
 if (transaction==0)  { return 0; }
 if (transaction->incomingHeader.failed) { AmmServer_Error("Will not try to grow a failed Header \n"); return 0; }

  struct HTTPHeader * hdr =  &transaction->incomingHeader;
  if (hdr == 0 )      { AmmServer_Error("Cannot grow header on transaction with no header.."); return 0; }

  struct AmmServer_Instance * instance = transaction->instance;
  if (instance == 0 ) { AmmServer_Error("Cannot grow header on transaction with no registered server instance.."); return 0; }
  //----------------------------------------------------------------------------------------------

  //We need to remember our old address!
  //In case realloc changes our base address we will need to recalculate all of our pointers..
  char * oldHeaderAddress    = hdr->headerRAW;
  unsigned int oldHeaderSize = hdr->MAXheaderRAWSize;

  //The new wannabe header size is defined by our previous plus the growth step from our configurations
  unsigned int wannabeHeaderSize = hdr->MAXheaderRAWSize + HTTP_POST_GROWTH_STEP_REQUEST_HEADER;

  //If we know where we are headerd..
  if (hdr->headerRAWRequestedSize!=0)
  {
   //And if we are still under the maximum POST transaction header size
   if (hdr->headerRAWRequestedSize <= instance->settings.MAX_POST_TRANSACTION_SIZE)
   {
    wannabeHeaderSize = hdr->headerRAWRequestedSize ; // + transaction->incomingHeader.headerHeadSize;
   } else
   {
    AmmServer_Error(
                    "Configuration does not allow growing the incoming header accomodation memory any more ( now %u / limit %u )",
                    hdr->headerRAWRequestedSize,
                    instance->settings.MAX_POST_TRANSACTION_SIZE
                   );
    AmmServer_Error("This header should be probably handled by dumping to /tmp/files");
    hdr->dumpedToFile=1;
    return 0;
   }
  }


  //First time we grow header ( means we are allocating it )
  if (oldHeaderAddress==0)
  {
    AmmServer_Success("First time to grow header..\n");

    hdr->headerRAW = (char * )  realloc (hdr->headerRAW , sizeof(char) * (wannabeHeaderSize+1) );
     if (hdr->headerRAW!=0 )
     {
       hdr->MAXheaderRAWSize = wannabeHeaderSize;
       memset(hdr->headerRAW,0,wannabeHeaderSize+1);
       return 1;
     } else
     {
       AmmServer_Error("Failed to allocate header space..!\n");
       return 0;
     }
  }





 //There is still room for incrementing the size of the buffer
  if (hdr->MAXheaderRAWSize < instance->settings.MAX_POST_TRANSACTION_SIZE )
   {
     //We will not allow the header to be any more than our settings allow..!
     if (wannabeHeaderSize > instance->settings.MAX_POST_TRANSACTION_SIZE )
                { wannabeHeaderSize = instance->settings.MAX_POST_TRANSACTION_SIZE; }



     //We will not allow the header to be any more than our settings allow..!
     char  * newBuffer = (char * )  realloc (hdr->headerRAW , sizeof(char) * (wannabeHeaderSize+1) );
     if (newBuffer!=0 )
     {
       newBuffer[wannabeHeaderSize]=0; //First of all null terminate the new header space..
       newBuffer[wannabeHeaderSize+1]=0; //First of all null terminate the new header space..
       hdr->headerRAW=newBuffer;
       hdr->MAXheaderRAWSize = wannabeHeaderSize;

       //Clean up new part of extra memory
       memset(newBuffer+oldHeaderSize,0,wannabeHeaderSize-oldHeaderSize);

       //In case realloc moved us to a new memory location we need to recalculate the header fields..
       if (oldHeaderAddress!=newBuffer)
       {
         recalculateHeaderFieldsBasedOnANewBaseAddress(oldHeaderAddress,transaction);
       }

       return 1;
     } else
     {
       AmmServer_Error("Failed to grow header , did we just run out of memory ?\n");
       AmmServer_Error("This header should be probably handled by dumping to /tmp/files");
       hdr->dumpedToFile=1;
     }
    } else
    {
      AmmServer_Error("Failed to grow header , we reached max header limit\n");
      AmmServer_Error("This header should be probably handled by dumping to /tmp/files");
      hdr->dumpedToFile=1;
    }

  return 0;
}





int keepAnalyzingHTTPHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
  fprintf(stderr,"keepAnalyzingHTTPHeader checks\n");
  if (instance==0)          { return 0; }
  if (transaction==0)       { return 0; }

  struct HTTPHeader * output  = &transaction->incomingHeader;

  //We will not waste our time with failed requests..
  if (output==0)                { return 0; }
  if (output->headerRAW==0)     { return 0; }
  if (output->headerRAWSize==0) { return 0; }


  char * webserver_root = instance->webserver_root;
  char * request = output->headerRAW + output->parsingStartOffset;
  char * startOfNewLine=request;
  unsigned int requestLength = transaction->incomingHeader.headerRAWSize - output->parsingStartOffset;

  //--------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------

  //In case our headerRAWSize is bigger than our allocation this is wrong and we should correct it here..
  if (requestLength>transaction->incomingHeader.MAXheaderRAWSize)
     {
        requestLength=transaction->incomingHeader.MAXheaderRAWSize;
        warningID(ASV_WARNING_REACHED_MAX_BUFFER_SIZE);
     }

  unsigned int i=0,newLineLength=0;


  while  (i<requestLength)
   {
     //fprintf(stderr,"steady\n");
     unsigned char r = request[i];
     //fprintf(stderr,"go\n");

     switch (r)
     {
        //If we reached a CR or LF character we might found a new line!
        case 13 :
        case 10 :
                  if (newLineLength>0)
                  {
                    //We've reached keepAnalyzingHTTPHeadera new line! , lets process the previous one
                    ++output->parsingCurrentLine;

                      AnalyzeHTTPLineRequest(
                                              instance,
                                              output,
                                              startOfNewLine,
                                              newLineLength,
                                              output->parsingCurrentLine,
                                              webserver_root
                                            );

                    output->parsingStartOffset+=newLineLength; //Remember where we are
                    newLineLength=0;

                    //BUG HERE ?
                    startOfNewLine = request+(i+1); //+1 gets past current CR or LF
                    r = *startOfNewLine;
                    switch (r)
                      { //Some hosts transmit CR LF so lets test for a second character
                        case 13 :
                        case 10 :
                             ++startOfNewLine;
                             ++i;
                        break;
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

     fprintf(stderr,"Our header content length is %lu , we got %u bytes at a buffer of %u bytes \n" , transaction->incomingHeader.ContentLength , transaction->incomingHeader.headerRAWSize , transaction->incomingHeader.MAXheaderRAWSize );
     if (transaction->incomingHeader.ContentLength> instance->settings.MAX_POST_TRANSACTION_SIZE)
     {
       fprintf(stderr,"Requested POST Size is too big calling it a day if we got the initial..!");
       transaction->incomingHeader.failed=1;
       return 1;
     } else
     if (totalHTTPRecvSize >  transaction->incomingHeader.headerRAWSize )
     {
       AmmServer_Warning(
                          "Header needs more space ( Recvd = %u Bytes / Header size = %u Bytes / MAX allowed size = %u Bytes )..!",
                          totalHTTPRecvSize,
                          transaction->incomingHeader.headerRAWHeadSize ,
                          transaction->incomingHeader.MAXheaderRAWSize
                        );

       if ( (!instance->settings.ENABLE_POST)||(!MASTER_ENABLE_POST) )
          {
            AmmServer_Warning("POST functionality is not enabled in this build , so allocated space for header ( MAX_HTTP_REQUEST_HEADER )  is small , this explains what happened..!\n");
          } else
          {
            AmmServer_Warning("You might consider tuning your AMMSET_MAX_POST_TRANSACTION_SIZE parameter to increase it ..!\n");
          }


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
