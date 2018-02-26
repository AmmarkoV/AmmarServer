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

#include "generic_header_tools.h"
#include "http_header_analysis.h"
#include "post_header_analysis.h"
#include "../network/file_server.h"
#include "../tools/http_tools.h"
#include "../tools/logs.h"
#include "../server_configuration.h"
#include "../stringscanners/httpHeader.h"
#include "../stringscanners/firstLines.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CR 13
#define LF 10


void printRecvError()
{
switch (errno)
      {
        //case EAGAIN :
        case EWOULDBLOCK :
         warning("The socket is marked nonblocking and the receive operation would block, or a receive timeout had been set and the timeout expired before data was received. POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value, so a portable application should check for both possibilities.");
        break;
        case EBADF : warning("The argument sockfd is an invalid descriptor."); break;
        case ECONNREFUSED : warning("A remote host refused to allow the network connection (typically because it is not running the requested service)."); break;
        case EFAULT : warning("The receive buffer pointer(s) point outside the process's address space.");  break;
        case EINTR : warning("The receive was interrupted by delivery of a signal before any data were available; see signal(7).");  break;
        case EINVAL : warning("Invalid argument passed.");  break;
        case ENOMEM : warning("Could not allocate memory for recvmsg()."); break;
        case ENOTCONN : warning("The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2))."); break;
        case ENOTSOCK : warning("The argument sockfd does not refer to a socket.");  break;
      };
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
         while ( (s<request_length)&&(request[s]==' ') ) { ++s; }
         if (s>=request_length) { fprintf(stderr,"Error #1 with GET/HEAD request\n"); return 0;}
         unsigned int e=s;
         while ( (e<request_length)&&(request[e]!=' ') ) { ++e; }
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
             //In case we don't have any attributes ..
             output->GETRequest = 0;

             if ( StripGETRequestQueryAndFragment(stripped/*,output->GETquery*/,MAX_QUERY) )
               {
                 //Now we should be pointing to the correct place of headerRAW
                 output->GETRequest = stripped + strlen(stripped) +1 ;
                 //fprintf(stderr,"Found a query , %s , resource is now %s \n",output->GETRequest,stripped);
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
 //What we have initially is like
 //Range: bytes=0-1024
 //But what we get here is only the last part i.e.
 // bytes=0- etc

 if (requestLength==9)
 {
  if (strncmp(request," bytes=0-",9)==0)
  {
     fprintf(stderr,"ProcessRangeHTTPLine got a full range starting from zero\n");
     *rangeStart=0; *rangeEnd=0;
     return 1;
  }
 }

  fprintf(stderr,"Got ProcessRangeHTTPLine  %u \n", requestLength);

  //TODO : Fix this ,
  /** @bug : ProcessRangeHTTPLine , can be improved , it is not thoroughly tested*/

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

 if ( (startOfStart==0) && (startOfEnd==0) ) { error("Could not find range in range request , will respond with an incorrect range (?) ..! "); return 0; }

 int requestLengthMinus1 = requestLength-1;
 if (startOfEnd >= requestLengthMinus1 ) { /*This means we have a range like `Range: bytes=144687104-` i.e. Unknown ending*/ } else
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
         payload_start+=strlen("ACCEPT-ENCODING:");
         if ( CheckHTTPHeaderCategory(request,request_length,"DEFLATE",&payload_start) ) { output->supports_compression=1; } else
                                                                                         {
                                                                                           output->supports_compression=0;
                                                                                          warning("We found an accept-encoding header , but not the deflate method..\n");
                                                                                         }
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_COOKIE :
         payload_start+=strlen("COOKIE:");
         //output->cookieIndex=payload_start;
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
          //output->hostIndex=payload_start;
          output->host=request+payload_start;
          output->hostLength=request_length-payload_start;
          return 1;
        break;
        //--------------------------------------------------------------
        case HTTPHEADER_IF_NONE_MATCH :
         payload_start+=strlen("IF-NONE-MATCH:");
         //output->eTagIndex=payload_start;
         output->eTag=request+payload_start;
         output->eTagLength = request_length-payload_start;
         //This is a hacky way to get past the space and " character -> IF-NONE_MATCH: "19392391932"
         if (output->eTag[0]==' ') { ++output->eTag; --output->eTagLength; }
         if (output->eTag[0]=='\"') { ++output->eTag; --output->eTagLength; }
         if (output->eTag[output->eTagLength-1]=='\"') { --output->eTagLength; }
         //fprintf(stderr,"IFNONEMATCH (%s) Provides ETag (%s, %u length ) \n",request,output->eTag,output->eTagLength);
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
             //fprintf(stderr,"Ranges not implemented correctly yet! %s \n",request);
             if (!ProcessRangeHTTPLine(request+payload_start,request_length-payload_start,&output->range_start,&output->range_end))
             {
               fprintf(stderr,"Failed to process ranges will use %lu , %lu \n",output->range_start,output->range_end);
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
             //output->refererIndex=payload_start;
             output->referer=request+payload_start;
             output->refererLength=request_length-payload_start;
             return 1;
        break;
        //--------------------------------------------------------------
     };

     //If we are a POST request also consider POST possibilities , implemented in post_header_analysis.c
     if ( (MASTER_ENABLE_POST) && (output->requestType==POST) )
     {
        return AnalyzePOSTLineRequest(instance,output,request,request_length,lines_gathered );
     }


   }

  return 0;
}


