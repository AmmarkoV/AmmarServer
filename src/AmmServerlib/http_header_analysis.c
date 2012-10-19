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
#include "http_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CR 13
#define LF 10

/*
Quick Reference -> A TYPICAL POST MESSAGE WITH A BINARY FILE CONTENT!
---------------------------------------------------------------------------
POST /formtest.html HTTP/1.1
Host: 127.0.0.1:8080
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:16.0) Gecko/20100101 Firefox/16.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
DNT: 1
Connection: keep-alive
Referer: http://127.0.0.1:8080/formtest.html
Content-Type: application/x-www-form-urlencoded
Content-Length: 40 <- This is the binary find content
user=&vehicle=Bike&testfile=DSC01537.JPG <-This is the form data as a URL var encoding

---

POST /myloader/index.php HTTP/1.1
Host: ammar.gr
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:16.0) Gecko/20100101 Firefox/16.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
DNT: 1
Connection: keep-alive
Referer: http://ammar.gr/myloader/
Cookie: wp-settings-1=m5%3Do%26m6%3Dc%26m4%3Do%26editor%3Dhtml%26m0%3Do%26imgsize%3Dmedium; wp-settings-time-1=1344596242; myshop=2883roc4oeq2k75eudsj3oa257; PHPSESSID=l5jejlnkd1htdh5t1uh1aa5ma0; wordpress_test_cookie=WP+Cookie+check; wordpress_logged_in_b7b2f92af29b61c363b9691a65231742=admin%7C1350397864%7C29cae87a47979992a2d057d8fa006626
Content-Type: multipart/form-data; boundary=---------------------------8096311293393534011208767067
Content-Length: 5347975
-----------------------------8096311293393534011208767067
Content-Disposition: form-data; name="rawresponse"

NO
-----------------------------8096311293393534011208767067
Content-Disposition: form-data; name="uploadedfile"; filename="DSC_1967.JPG"
Content-Type: image/jpeg

ÿØÿáÿþExif
HTTP/1.1 200 OK
Date: Mon, 15 Oct 2012 21:52:39 GMT
Server: Apache/2.2.14 (Ubuntu)
X-Powered-By: PHP/5.3.2-1ubuntu4.17
Vary: Accept-Encoding
Content-Encoding: gzip
Content-Length: 1128
Keep-Alive: timeout=15, max=98
Connection: Keep-Alive
Content-Type: text/html


*/

int FreeHTTPRequest(struct HTTPRequest * output)
{
/* Getting a consistent segfault on the raspberry pi The last console line is
   ->  Freeing HTTP Request : ETag *** glibc detected *** src/ammarserver: free(): invalid next size (fast): 0x01ad7ac8 *** */

   fprintf(stderr,"Freeing HTTP Request : ");
   unsigned int fields_I_try_to_clean=0;

   fprintf(stderr,"ETag ");
   ++fields_I_try_to_clean; if (output->ETag !=0) { free(output->ETag); output->ETag=0; }

   fprintf(stderr,"Cookie ");
   ++fields_I_try_to_clean; if (output->Cookie!=0) { free(output->Cookie); output->Cookie=0; }

   fprintf(stderr,"Referer ");
   ++fields_I_try_to_clean; if (output->Referer!=0) { free(output->Referer); output->Referer=0; }

   fprintf(stderr,"Host ");
   ++fields_I_try_to_clean; if (output->Host!=0) { free(output->Host); output->Host=0; }

   fprintf(stderr,"UserAgent ");
   ++fields_I_try_to_clean; if (output->UserAgent!=0) { free(output->UserAgent); output->UserAgent=0; }

   fprintf(stderr,"ContentType ");
   ++fields_I_try_to_clean; if (output->ContentType!=0) { free(output->ContentType); output->ContentType=0; }
   //FIELDS_TO_CLEAR_FROM_HTTP_REQUEST is a way to remember to add things here every time I add a new field..!
   fprintf(stderr,".... ok\n");
    if (FIELDS_TO_CLEAR_FROM_HTTP_REQUEST!=fields_I_try_to_clean) { return 0; }
    return 1;
}

int HTTPRequestComplete(char * request,unsigned int request_length)
{
  /*  This call returns 1 when we find two subsequent newline characters
      which mark the ending of an HTTP header..! The function returns 1 or 0 ..! */

  if (request_length<2) { return 0; } // at least LF LF is expected :P

  fprintf(stderr,"Checking if request with %u chars is complete .. ",request_length);
  int i=request_length-1;
  while (i>1)
   {
      if ( request[i]==LF )
       {
        if (i>=1) { if (( request[i-1]==LF )&&( request[i]==LF )) { fprintf(stderr,"it is \n"); return 1; }  } /* unix 2x new line sequence */
        if (i>=3) { if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF )) { fprintf(stderr,"it is \n"); return 1; } } /* windows 2x new line sequence */
       }
     --i;
   }

   fprintf(stderr,"it isn't \n");
   return 0;
}

inline int ProcessFirstHTTPLine(struct HTTPRequest * output,char * request,unsigned int request_length, char * webserver_root)
{
     if (request_length<3)  { fprintf(stderr,"A very small first line \n "); return 0; }
     // The firs line should contain the message type so .. lets see..!
     if (
          ((request[0]=='G')&&(request[1]=='E')&&(request[2]=='T')) ||
          ((request[0]=='H')&&(request[1]=='E')&&(request[2]=='A')&&(request[3]=='D')) ||
          ((request[0]=='P')&&(request[1]=='O')&&(request[2]=='S')&&(request[3]=='T')) //POST REQUEST DOESNT REALLY BELONG HERE , BUT TO SAVE SPACE AND EFFORT IT IS TREATED LIKE GET/HEAD
        )
       { // A GET or HEAD Request..!

         unsigned int s=3; //Initial position past GET/HEAD
         if ((request[0]=='G')&&(request[1]=='E')&&(request[2]=='T')) {  fprintf(stderr,"GET Request %s\n", request); output->requestType=GET; s=3; } else
         if ((request[0]=='H')&&(request[1]=='E')&&(request[2]=='A')&&(request[3]=='D')) {  fprintf(stderr,"HEAD Request %s\n", request); output->requestType=HEAD; s=4; }
         if ((request[0]=='P')&&(request[1]=='O')&&(request[2]=='S')&&(request[3]=='T')) {  fprintf(stderr,"POST Request %s\n", request); output->requestType=POST; s=4; }

         while ( (request[s]==' ')&&(s<request_length) ) { ++s; }
         if (s>=request_length) { fprintf(stderr,"Error #1 with GET/HEAD request\n"); return 0;}
         unsigned int e=s;
         while ( (request[e]!=' ')&&(e<request_length) ) { ++e; }
         if (e>=request_length) { fprintf(stderr,"Error #2 with GET/HEAD request\n"); return 0;}
         request[e]=0; //Signal ending

         char * stripped = &request[s];
         //fprintf(stderr,"Stripped GET/HEAD request is %s \n",stripped);
         StripHTMLCharacters_Inplace(stripped); // <- This converts char sequences like %20 to " " it HAS to be done before filename stripper to ensure string safety

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
                 fprintf(stderr,"Found a query , %s , resource is now %s \n",output->GETquery,stripped);
               }

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

       } else
     if ((request[0]=='P')&&(request[1]=='U')&&(request[2]=='T'))                                                                             { /* A PUT Request..!     */ fprintf(stderr,"PUT Request %s\n", request);     output->requestType=PUT; } else
     if ((request[0]=='D')&&(request[1]=='E')&&(request[2]=='L')&&(request[3]=='E')&&(request[4]=='T')&&(request[5]=='E'))                    { /* A DELETE Request..!  */ fprintf(stderr,"DELETE Request %s\n", request);  output->requestType=DELETE; } else
     if ((request[0]=='T')&&(request[1]=='R')&&(request[2]=='A')&&(request[3]=='C')&&(request[4]=='E'))                                       { /* A TRACE Request..!   */ fprintf(stderr,"TRACE Request %s\n", request);   output->requestType=TRACE; } else
     if ((request[0]=='O')&&(request[1]=='P')&&(request[2]=='T')&&(request[3]=='I')&&(request[4]=='O')&&(request[5]=='N')&&(request[6]=='S')) { /* A OPTIONS Request..! */ fprintf(stderr,"OPTIONS Request %s\n", request); output->requestType=OPTIONS; } else
     if ((request[0]=='C')&&(request[1]=='O')&&(request[2]=='N')&&(request[3]=='N')&&(request[4]=='E')&&(request[5]=='C')&&(request[6]=='T')) { /* A CONNECT Request..! */ fprintf(stderr,"CONNECT Request %s\n", request); output->requestType=CONNECT; } else
     if ((request[0]=='P')&&(request[1]=='A')&&(request[2]=='T')&&(request[3]=='C')&&(request[4]=='H'))                                       { /* A PATCH Request..!   */ fprintf(stderr,"PATCH Request %s\n", request);   output->requestType=PATCH; }

  return 1;
}


inline int ProcessAuthorizationHTTPLine(struct HTTPRequest * output,char * request,unsigned int request_length, char * webserver_root,unsigned int * payload_pos)
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
          if (strcmp(BASE64PASSWORD,payload)==0) { output->authorized=1; } else
                                                 { output->authorized=0; }
         }
    return 1;
}



int AnalyzeHTTPLineRequest(struct HTTPRequest * output,char * request,unsigned int request_length,unsigned int lines_gathered, char * webserver_root)
{
  /*
      This call fills in the output variable according to the line data held in the request string..!
      it is made to be called internally by AnalyzeHTTPRequest
  */

  //fprintf(stderr,"Analyzing HTTP Request : Line %u , `%s` \n",lines_gathered,request);

  if (lines_gathered==1)
   { //Process first line , GET / HEAD / POST etc ,  this is a pretty lengthy call so it is split in a seperate function
     return ProcessFirstHTTPLine(output,request,request_length,webserver_root);
   } else
   {
     unsigned int payload_start = 0;

     if ((PASSWORD_PROTECTION)&&(BASE64PASSWORD!=0))
       { //Consider password protection header sections..!
        if ( CheckHTTPHeaderCategory(request,request_length,"AUTHORIZATION:",&payload_start) )
           {
             return ProcessAuthorizationHTTPLine(output,request,request_length,webserver_root,&payload_start);
           }
       }


     if (output->requestType==POST)
      {
          //If we just had a POST request , it  may have a file associated with it , so we will check for content tags..
          //Scanning for the case that the line is -> Content-Type: (i.e.) application/x-www-form-urlencoded
         if ( CheckHTTPHeaderCategory(request,request_length,"CONTENT-TYPE:",&payload_start) )
          {
            if (output->ContentType!=0) { free(output->ContentType); output->ContentType=0; }
            output->ContentType=GetNewStringFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            if (output->ContentType==0) { return 0; } else { return 1;}
          }

         //Scanning for the case that the line is -> Content-Length: This means a file has been appended
         if ( CheckHTTPHeaderCategory(request,request_length,"CONTENT-LENGTH:",&payload_start) )
          {
            output->ContentLength = GetIntFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            return (output->ContentLength!=0);
          }
      }



      if ( CheckHTTPHeaderCategory(request,request_length,"RANGE:",&payload_start) )
          {
             //Todo here : Fill in range_start and range_end
             output->range_start=0;
             output->range_end=0;
             return 0;
          }


      /*REFERRER AND REFERER ARE THE SAME CASE , THE RFC HAS THE MISPELLED VERSION OF THE WORD , BOTH OF THEM EXIST IN THE WILD :P*/
      if ( CheckHTTPHeaderCategory(request,request_length,"REFERRER:",&payload_start) )
          {
            if (output->Referer!=0) { free(output->Referer); output->Referer=0; }
            output->Referer=GetNewStringFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            if (output->Referer==0) { return 0; } else { return 1;}
          } else
      if ( CheckHTTPHeaderCategory(request,request_length,"REFERER:",&payload_start) ) // <- The spec Referrer string is wrong :P
          {
            if (output->Referer!=0) { free(output->Referer); output->Referer=0; }
            output->Referer=GetNewStringFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            if (output->Referer==0) { return 0; } else { return 1;}
          }
       /*---------------------------------------------------------------------------------------------------------------------*/

      if ( CheckHTTPHeaderCategory(request,request_length,"HOST:",&payload_start) )
          {
            if (output->Host!=0) { free(output->Host); output->Host=0; }
            output->Host=GetNewStringFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            if (output->Host==0) { return 0; } else { return 1;}
          }

      if ( CheckHTTPHeaderCategory(request,request_length,"USER-AGENT:",&payload_start) )
          {
            if (output->UserAgent!=0) { free(output->UserAgent); output->UserAgent=0; }
            output->UserAgent=GetNewStringFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            if (output->UserAgent==0) { return 0; } else { return 1;}
          }


      if ( CheckHTTPHeaderCategory(request,request_length,"COOKIE:",&payload_start) )
          {
            if (output->Cookie!=0) { free(output->Cookie); output->Cookie=0; }
            output->Cookie=GetNewStringFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            if (output->Cookie==0) { return 0; } else { return 1;}
          }

      if ( CheckHTTPHeaderCategory(request,request_length,"CONNECTION:",&payload_start) )
          {
            if (CheckHTTPHeaderCategory(request,request_length,"KEEP-ALIVE",&payload_start)) { output->keepalive=1; fprintf(stderr,"KeepAlive is set\n"); return 1;}
            return 0;
          }

      //If-None-Match: "3e0f0d-1485-4c2646d587b7d"
      if ( CheckHTTPHeaderCategory(request,request_length,"IF-NONE-MATCH:",&payload_start) )
          {
            if (output->ETag!=0) { free(output->ETag); output->ETag=0; }
            output->ETag=GetNewStringFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
            if (output->ETag==0) { return 0; } else { return 1;}
          }

      //If-Modified-Since: Thu, 14 Jun 2012 01:14:53 GMT
      if ( CheckHTTPHeaderCategory(request,request_length,"IF-MODIFIED-SINCE:",&payload_start) )
          { fprintf(stderr,"304 Not Modified headers through dates not supported yet\n"); return 0; }


   }
  //Todo check for keepalive header option here and output->keepalive=1;
  return 1;
}

int AnalyzeHTTPRequest(struct HTTPRequest * output,char * request,unsigned int request_length, char * webserver_root)
{
  /*
      This call fills in the output variable according by subsequent calls to the AnalyzeHTTPLineRequest function
      the code here just serves as a line parser for AnalyzeHTTPLineRequest
  */
  //fprintf(stderr,"Starting a fresh HTTP Request Analysis\n");
  memset(output,0,sizeof(struct HTTPRequest)); // Clear output http request
  output->requestType=NONE; // invalidate current output data..!
  output->range_start=0;
  output->range_end=0;
  output->authorized=0;

  char line[MAX_HTTP_REQUEST_HEADER_LINE+1]={0};
  unsigned int i=0,chars_gathered=0,lines_gathered=0;
  while  ( (i<request_length)&&(i<MAX_HTTP_REQUEST_HEADER_LINE) )
   {
     if  ( ((request[i]==CR)||(request[i]==LF)) && (chars_gathered>0) )
      {
        line[chars_gathered]=0;

        //We've got ourselves a new line!
        ++lines_gathered;
        AnalyzeHTTPLineRequest(output,line,strlen(line),lines_gathered,webserver_root);
        line[0]=0; //line is "cleared" :P
        chars_gathered=0;
      }
        else
      {
        line[chars_gathered]=request[i];
        ++chars_gathered;
      }

     ++i;
   }

  return 1;
}
