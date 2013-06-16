/*
AmmarServer , HTTP Server Library

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "http_header_analysis.h"
#include "post_header_analysis.h"
#include "../tools/http_tools.h"
#include "../tools/logs.h"
#include "../server_configuration.h"
#include "../stringscanners/httpHeader.h"
#include "../stringscanners/firstLines.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CR 13
#define LF 10





char * ReceiveHTTPHeader(struct AmmServer_Instance * instance,int clientSock , unsigned long * headerLength)
{
 *headerLength=0;
 int opres=0;
 unsigned int incomingRequestLength = 0 ;
 unsigned int MAXincomingRequestLength = MAX_HTTP_REQUEST_HEADER+1 ;
 char  * incomingRequest = (char*)  malloc(sizeof(char) * (MAXincomingRequestLength) );
 if (incomingRequest==0) { error("Could not allocate enough memory for Header "); return 0; }
 incomingRequest[0]=0;

 fprintf(stderr,"KeepAlive Server Loop , Waiting for a valid HTTP header..\n");
 while (
        (HTTPHeaderComplete(incomingRequest,incomingRequestLength)==0) &&
        (instance->server_running)
       )
 {
  //Gather Header until http request contains two newlines..!
  opres=recv(clientSock,&incomingRequest[incomingRequestLength],MAXincomingRequestLength-incomingRequestLength,0);
  if (opres<=0) { free(incomingRequest); return 0;   /*TODO : Check opres here..!*/ } else
    {
      incomingRequestLength+=opres;
      fprintf(stderr,"Got %u bytes ( %u total )\n",opres,incomingRequestLength);
      if (incomingRequestLength>=MAXincomingRequestLength)
      {
         //We filled our buffer , if we have a POST request there is a different limit
         //so that we can upload files
         if ( ( HTTPHeaderIsPOST(incomingRequest,incomingRequestLength ) ) && ( ENABLE_POST ) )
          {
            //unsigned long oldLimit = MAXincomingRequestLength;
            if (MAXincomingRequestLength < MAX_HTTP_POST_REQUEST_HEADER )
            {
              MAXincomingRequestLength += HTTP_POST_GROWTH_STEP_REQUEST_HEADER;
              if (MAXincomingRequestLength > MAX_HTTP_POST_REQUEST_HEADER )
                   { MAXincomingRequestLength = MAX_HTTP_POST_REQUEST_HEADER; }


              char  * largerRequest = (char * )  realloc (incomingRequest, MAXincomingRequestLength );
              if ( incomingRequest != largerRequest ) { fprintf(stderr,"Successfully grown input header using %u/%u bytes\n",incomingRequestLength,MAXincomingRequestLength); }
                                                        else
                                                      {
                                                        fprintf(stderr,"The request would overflow POST limit , dropping client \n");
                                                        free(incomingRequest);
                                                        return 0;
                                                      }
            }
          } else
          {
            fprintf(stderr,"The request would overflow , dropping client \n");
            free(incomingRequest);
            return 0;
          }
      }
    }
  }
  fprintf(stderr,"Finished Waiting for a valid HTTP header..\n");
  *headerLength=incomingRequestLength;
  return incomingRequest;
}


int AppendPOSTRequestToHTTPHeader(struct HTTPTransaction * transaction)
{
 unsigned int incomingRequestLength = transaction->incomingHeader.headerRAWSize;

 transaction->incomingHeader.headerRAWSize  = HTTPHeaderComplete(transaction->incomingHeader.headerRAW,transaction->incomingHeader.headerRAWSize);
 fprintf(stderr,"Header Size  =  %u / %u \n",transaction->incomingHeader.headerRAWSize,transaction->incomingHeader.headerRAWSize);
 transaction->incomingHeader.POSTrequestSize = transaction->incomingHeader.headerRAWSize;
 return 1;
}






int FreeHTTPHeader(struct HTTPHeader * output)
{
/* Getting a consistent segfault on the raspberry pi The last console line is
   ->  Freeing HTTP Request : ETag *** glibc detected *** src/ammarserver: free(): invalid next size (fast): 0x01ad7ac8 *** */

   unsigned int fields_I_try_to_clean=0;

     //This does nothing any more , to be removed

   return 1;
}

int HTTPHeaderComplete(char * request,unsigned int request_length)
{
  /*  This call returns 1 when we find two subsequent newline characters
      which mark the ending of an HTTP header..! The function returns 1 or 0 ..! */

  if (request_length<2) { return 0; } // at least LF LF is expected :P

  fprintf(stderr,"Checking if request with %u chars is complete .. ",request_length);
  unsigned int i=request_length-1;
  while (i>1)
   {
      if ( request[i]==LF )
       {
        if (i>=1) { if (( request[i-1]==LF )&&( request[i]==LF )) { fprintf(stderr,"it is \n"); return i; }  } /* unix 2x new line sequence */
        if (i>=3) { if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF )) { fprintf(stderr,"it is \n"); return i; } } /* windows 2x new line sequence */
       }
     --i;
   }

   fprintf(stderr,"it isn't \n");
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


int ProcessFirstHTTPLine(struct HTTPHeader * output,char * request,unsigned int request_length, char * webserver_root)
{
     if (request_length<3)  { fprintf(stderr,"A very small first line \n "); return 0; }

      unsigned int requestType  = scanFor_firstLines(request,request_length);
      unsigned int s=3; //Initial position past GET/HEAD

      switch (requestType)
      {
       case FIRSTLINES_GET     :  output->requestType=GET;     s=3; break ;
       case FIRSTLINES_HEAD    :  output->requestType=HEAD;    s=4; break ;
       case FIRSTLINES_POST    :  output->requestType=POST;    s=4; break ;
       case FIRSTLINES_PUT     :  output->requestType=PUT;     s=3; break ;
       case FIRSTLINES_DELETE  :  output->requestType=DELETE;  s=6; break ;
       case FIRSTLINES_TRACE   :  output->requestType=TRACE;   s=5; break ;
       case FIRSTLINES_OPTIONS :  output->requestType=OPTIONS; s=7; break ;
       case FIRSTLINES_CONNECT :  output->requestType=CONNECT; s=7; break ;
       case FIRSTLINES_PATCH   :  output->requestType=PATCH;   s=5; break ;
      };

     // The firs line should contain the message type so .. lets see..!
     if (
          (output->requestType==GET) ||
          (output->requestType==HEAD) ||
          (output->requestType==POST) //POST REQUEST DOESNT REALLY BELONG HERE , BUT TO SAVE SPACE AND EFFORT IT IS TREATED LIKE GET/HEAD
        )
       { // A GET or HEAD Request..!
         while ( (request[s]==' ')&&(s<request_length) ) { ++s; }
         if (s>=request_length) { fprintf(stderr,"Error #1 with GET/HEAD request\n"); return 0;}
         unsigned int e=s;
         while ( (request[e]!=' ')&&(e<request_length) ) { ++e; }
         if (e>=request_length) { fprintf(stderr,"Error #2 with GET/HEAD request\n"); return 0;}
         request[e]=0; //Signal ending

         //We move forward to the GET request , stripped will contain the stripped resource requested
         //after stripping GET Query and Fragment and raw html characters
         char * stripped = &request[s];

         if (strlen(stripped)>=MAX_RESOURCE-2)
           {
             fprintf(stderr,"Warning : GET/HEAD request is too big , dropping request..! \n");
             output->requestType=BAD;
             return 0;
           } else
           {
             /*!Input String Verification from client
              Since this string will be passed to fopen this can be dangerous so we perform some
              security checks with FilenameStripperOk to make sure no escape characters subdirs out of
              public_html ( via public_html/../etc ) and overflows may happen..!
              Most of the functions are implemented in http_tools!
              The results are then copied to output->resource and output->verified_local_resource which contain
              the resource requested as the client stated it and as we verified for local filesystem..!
              */
             if ( StripGETRequestQueryAndFragment(stripped,output->GETquery,MAX_QUERY) )
               {
                 StripHTMLCharacters_Inplace(output->GETquery,0 /* 0 = Disregard dangerous bytes , Safety OFF*/); // <- This call converts char sequences like %20 to " " and %00 to \0 disregarding any form of safety , ( since it is a raw var )
                 fprintf(stderr,"Found a query , %s , resource is now %s \n",output->GETquery,stripped);
               }

             StripHTMLCharacters_Inplace(stripped,1 /* 1 = filter dangerous bytes , File Safety ON*/); // <- This call converts char sequences like %20 to " " but %00 becomes _00 ( instead of \0 ) , it HAS to be done before filename stripper to ensure string safety

             if (FilenameStripperOk(stripped))
              {
                strncpy(output->resource,stripped,MAX_RESOURCE);
                strncpy(output->verified_local_resource,webserver_root,MAX_FILE_PATH);
                strncat(output->verified_local_resource,stripped,MAX_FILE_PATH);
                ReducePathSlashes_Inplace(output->verified_local_resource);
                return 1;
              } else
              {
                fprintf(stderr,"Warning : Suspicious request , dropping it ..! \n");
                output->requestType=BAD;
                return 0;
              }
           }
         return 1;
       }

  return 0;
}


inline int ProcessAuthorizationHTTPLine(struct AmmServer_Instance * instance,struct HTTPHeader * output,char * request,unsigned int request_length,unsigned int * payload_pos)
{
        unsigned int payload_start = *payload_pos;
        //fprintf(stderr,"Got an authorization string , whole line is %s \n",request);
        //It is an authorization line , typically like ->  `Authorization: Basic YWRtaW46YW1tYXI=`
        //TODO : this needs some thought , it can be improved!
        payload_start+=1;
        while ( (payload_start<request_length) && (request[payload_start]==' ') ) { ++payload_start; }
        payload_start+=strlen("Basic");
        while ( (payload_start<request_length) && (request[payload_start]==' ') ) { ++payload_start; }

        if (payload_start<request_length)
         {
          trim_last_empty_chars(request,request_length);
          char * payload = &request[payload_start];
          fprintf(stderr,"Got an authorization string -> `%s` \n",payload);
          //fprintf(stderr,"Got an authorization string -> `%s` , ours is `%s`\n",payload,BASE64PASSWORD);
          if (strcmp(instance->settings.BASE64PASSWORD,payload)==0) { output->authorized=1; } else
                                                                      { output->authorized=0; }
         }
    return 1;
}


inline int ProcessRangeHTTPLine(char * request,unsigned int requestLength,unsigned long * rangeStart,unsigned long * rangeEnd)
{
 //Range: bytes=0-1024
 fprintf(stderr,"Got ProcessRangeHTTPLine %s , %u \n",request,requestLength);

 int startOfStart=0;
 int startOfEnd=0;
 int dashPos=0;

 int i=requestLength;
 while (i>0)
 {
   if (request[i]=='-') {  startOfEnd=i+1; dashPos=i; } else
   if (request[i]=='=') {  startOfStart=i+1; }
   --i;
 }

 if ( (startOfStart==0) && (startOfEnd==0) ) { warning("Could not find range in range request"); return 0; }
 if (startOfEnd>=requestLength-1 ) { /*This means we have a range like `Range: bytes=144687104-` i.e. Unknown ending*/ } else
                                   { *rangeEnd = atoi(request+startOfEnd); }

 request[dashPos]=0;
 *rangeStart = atoi(request+startOfStart);
 request[dashPos]='-';

 fprintf(stderr,"Resolved Range is %lu to %lu\n",*rangeStart,*rangeEnd);
 return 1;
}


int AnalyzeHTTPLineRequest(
                            struct AmmServer_Instance * instance,
                            struct HTTPHeader * output,
                            char * request,
                            unsigned int request_length,
                            unsigned int lines_gathered,
                            char * webserver_root
                          )
{
  /*
      This call fills in the output variable according to the line data held in the request string..!
      it is made to be called internally by AnalyzeHTTPHeader
  */
  //fprintf(stderr,"Analyzing HTTP Request : Line %u , `%s` \n",lines_gathered,request);

  if (lines_gathered==1)
   { //Process first line , GET / HEAD / POST etc ,  this is a pretty lengthy call so it is split in a seperate function
     return ProcessFirstHTTPLine(output,request,request_length,webserver_root);
   } else
   {
     unsigned int payload_start = 0;

     unsigned int requestType = scanFor_httpHeader(request,request_length);
     //fprintf(stderr,"Thinking about string (%s) starts with %c and %c  got back %u \n",request,request[0],request[1] , requestType);
     switch (requestType)
     {
        case HTTPHEADER_AUTHORIZATION :
         if ((instance->settings.PASSWORD_PROTECTION)&&(instance->settings.BASE64PASSWORD!=0))
         {
           payload_start+=strlen("AUTHORIZATION:");
           return ProcessAuthorizationHTTPLine(instance,output,request,request_length,&payload_start);
         }
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_ACCEPT_ENCODING :
            /*
         payload_start+=strlen("ACCEPT-ENCODING:");
         if ( CheckHTTPHeaderCategory(request,request_length,"DEFLATE",&payload_start) ) { output->supports_compression=1; } else
                                                                                         {
                                                                                           output->supports_compression=0;
                                                                                           fprintf(stderr,"We found an accept-encoding header , but not the deflate method..\n");
                                                                                         }
           */
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_COOKIE :
         payload_start+=strlen("COOKIE:");
         output->cookie=request+payload_start;
         output->cookieLength = request_length-payload_start;
         return 1;
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_CONNECTION :
         payload_start+=strlen("CONNECTION:");
         if (CheckHTTPHeaderCategory(request,request_length,"KEEP-ALIVE",&payload_start)) { output->keepalive=1; fprintf(stderr,"KeepAlive is set\n"); return 1;}

        break;
        //--------------------------------------------------------------
        case HTTPHEADER_HOST :
          payload_start+=strlen("HOST:");
          output->host=request+payload_start;
          output->hostLength=request_length-payload_start;
          return 1;
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_IF_NONE_MATCH :
         payload_start+=strlen("IF-NONE-MATCH:");
         output->eTag=request+payload_start;
         output->eTagLength = request_length-payload_start;
         return 1;
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_IF_MODIFIED_SINCE :
            payload_start+=strlen("IF-MODIFIED-SINCE:");
            fprintf(stderr,"304 Not Modified headers through dates not supported yet\n"); return 0;
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_RANGE :
             //Todo here : Fill in range_start and range_end
             payload_start+=strlen("RANGE:");
             fprintf(stderr,"Ranges not implemented yet! %s \n",request);
             if (!ProcessRangeHTTPLine(request+payload_start,request_length-payload_start,&output->range_start,&output->range_end))
             {
               output->range_start=0;
               output->range_end=0;
               return 0;
             }
             return 1;
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_REFERRER : //The same case as HTTPHEADER_REFERER
            payload_start+=strlen("REFERRER:");
        case HTTPHEADER_REFERER :
             if (HTTPHEADER_REFERER==requestType) {payload_start+=strlen("REFERER:");  }
             output->referer=request+payload_start;
             output->refererLength=request_length-payload_start;
             return 1;
        break;
        //--------------------------------------------------------------
     };


     if (output->requestType==POST)
     {
        return AnalyzePOSTLineRequest(instance,output,request,request_length,lines_gathered,webserver_root );
     }


   }

  return 0;
}

//int AnalyzeHTTPHeader(struct AmmServer_Instance * instance,struct HTTPHeader * output,char * request,unsigned int request_length, char * webserver_root)
int AnalyzeHTTPHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
  struct HTTPHeader * output  = &transaction->incomingHeader;
  char * request = transaction->incomingHeader.headerRAW;
  unsigned int request_length = transaction->incomingHeader.headerRAWSize;
  char * webserver_root = instance->webserver_root;


  /*
      This call fills in the output variable according by subsequent calls to the AnalyzeHTTPLineRequest function
      the code here just serves as a line parser for AnalyzeHTTPLineRequest
  */

  memset(output,0,sizeof(struct HTTPHeader)); // Clear output http request
  output->requestType=NONE; // invalidate current output data..!
  output->range_start=0;
  output->range_end=0;
  output->authorized=0;

  fprintf(stderr,"Started New Analyzing Header\n");

  char * preciseLine;
  char line[MAX_HTTP_REQUEST_HEADER_LINE+1]={0};
  char lineUpcase[MAX_HTTP_REQUEST_HEADER_LINE+1]={0};
  unsigned int i=0,chars_gathered=0,lines_gathered=0;
  while  ( (i<request_length)&&(i<MAX_HTTP_REQUEST_HEADER_LINE) )
   {
     switch (request[i])
     {
        //If we reached a CR or LF character we might found a new line!
        case CR :
        case LF :
                  if (chars_gathered>0)
                  {
                    line[chars_gathered]=0;
                    //We've got ourselves a new line!
                    ++lines_gathered;
                    preciseLine = line ;
                    switch (*preciseLine)
                      { case CR :
                        case LF : ++preciseLine;
                                  break;
                      };
                    //if ( (*preciseLine==10) || (*preciseLine==13) ) { ++preciseLine; }
                    AnalyzeHTTPLineRequest(instance,output,preciseLine,strlen(preciseLine),lines_gathered,webserver_root);
                    line[0]=0; //line is "cleared" :P
                    chars_gathered=0;
                    break;
                  }
        default :
                  line[chars_gathered]=request[i];
                  ++chars_gathered;
                 break;
     };

     ++i;
   }
 /*
   while  ( (i<request_length)&&(i<MAX_HTTP_REQUEST_HEADER_LINE) )
   {
     if  ( ((request[i]==CR)||(request[i]==LF)) && (chars_gathered>0) )
      {
        line[chars_gathered]=0;

        //We've got ourselves a new line!
        ++lines_gathered;

        preciseLine = line ;
        if ( (*preciseLine==10) || (*preciseLine==13) ) { ++preciseLine; }
        AnalyzeHTTPLineRequest(instance,output,preciseLine,strlen(preciseLine),lines_gathered,webserver_root);
        line[0]=0; //line is "cleared" :P
        chars_gathered=0;
      }
        else
      {
        line[chars_gathered]=request[i];
        ++chars_gathered;
      }

     ++i;
   }*/

  return 1;
}
