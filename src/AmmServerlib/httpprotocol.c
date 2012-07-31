#include "httpprotocol.h"

#include <stdio.h>
#include <stdlib.h>

#define CR 13
#define LF 10

enum TypesOfRequests
{
    HEAD=0,
    //Asks for the response identical to the one that would correspond to a GET request, but without the response body. This is useful for retrieving meta-information written in response headers, without having to transport the entire content.
    GET,
    //Requests a representation of the specified resource. Requests using GET should only retrieve data and should have no other effect. (This is also true of some other HTTP methods.)[1] The W3C has published guidance principles on this distinction, saying, "Web application design should be informed by the above principles, but also by the relevant limitations."[11] See safe methods below.
    POST,
    //Submits data to be processed (e.g., from an HTML form) to the identified resource. The data is included in the body of the request. This may result in the creation of a new resource or the updates of existing resources or both.
    PUT,
    //Uploads a representation of the specified resource.
    DELETE,
    //Deletes the specified resource.
    TRACE,
    //Echoes back the received request, so that a client can see what (if any) changes or additions have been made by intermediate servers.
    OPTIONS,
    //Returns the HTTP methods that the server supports for specified URL. This can be used to check the functionality of a web server by requesting '*' instead of a specific resource.
    CONNECT,
    //Converts the request connection to a transparent TCP/IP tunnel, usually to facilitate SSL-encrypted communication (HTTPS) through an unencrypted HTTP proxy.[12]
    PATCH
    //Is used to apply partial modifications to a resource.[13]
};



int HTTPRequestComplete(char * request,unsigned int request_length)
{
  int i=request_length-1;
  while (i>1)
   {
      if ( request[i]==LF )
       {
        if (i>=1)
          {
           if (( request[i-1]==LF )&&( request[i]==LF )) { return 1; }  // unix 2x new line sequence
          }

         if (i>=3)
          {
           if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF )) { return 1; } // windows 2x new line sequence
          }
       }
     --i;
   }
   return 0;
}

int AnalyzeHTTPLineRequest(struct HTTPRequest * output,char * request,unsigned int request_length,unsigned int lines_gathered)
{
  fprintf(stderr,"Analyzing HTTP Request : Line %u , `%s` \n",lines_gathered,request);

  if (lines_gathered==1)
   {
     if (request_length<3)  { fprintf(stderr,"A very small first line \n "); return 0; }
     // The firs line should contain the message type so .. lets see..!
     if ((request[0]=='G')&&(request[1]=='E')&&(request[2]=='T'))
       { // A GET Request..!
         fprintf(stderr,"GET Request %s\n", request);
       } else
     if ((request[0]=='H')&&(request[1]=='E')&&(request[2]=='A')&&(request[3]=='D'))
       { // A HEAD Request..!
         fprintf(stderr,"HEAD Request %s\n", request);
       } else
     if ((request[0]=='P')&&(request[1]=='O')&&(request[2]=='S')&&(request[3]=='T'))
       { // A POST Request..!
         fprintf(stderr,"POST Request %s\n", request);
       } else
     if ((request[0]=='P')&&(request[1]=='U')&&(request[2]=='T'))
       { // A PUT Request..!
         fprintf(stderr,"PUT Request %s\n", request);
       } else
     if ((request[0]=='D')&&(request[1]=='E')&&(request[2]=='L')&&(request[3]=='E')&&(request[4]=='T')&&(request[5]=='E'))
       { // A DELETE Request..!
         fprintf(stderr,"DELETE Request %s\n", request);
       } else
     if ((request[0]=='T')&&(request[1]=='R')&&(request[2]=='A')&&(request[3]=='C')&&(request[4]=='E'))
       { // A TRACE Request..!
         fprintf(stderr,"TRACE Request %s\n", request);
       } else
     if ((request[0]=='O')&&(request[1]=='P')&&(request[2]=='T')&&(request[3]=='I')&&(request[4]=='O')&&(request[5]=='N')&&(request[6]=='S'))
       { // A OPTIONS Request..!
         fprintf(stderr,"OPTIONS Request %s\n", request);
       } else
     if ((request[0]=='C')&&(request[1]=='O')&&(request[2]=='N')&&(request[3]=='N')&&(request[4]=='E')&&(request[5]=='C')&&(request[6]=='T'))
       { // A CONNECT Request..!
         fprintf(stderr,"CONNECT Request %s\n", request);
       } else
     if ((request[0]=='P')&&(request[1]=='A')&&(request[2]=='T')&&(request[3]=='C')&&(request[4]=='H'))
       { // A PATCH Request..!
         fprintf(stderr,"PATCH Request %s\n", request);
       }
   }


  return 0;
}

int AnalyzeHTTPRequest(struct HTTPRequest * output,char * request,unsigned int request_length)
{
  fprintf(stderr,"Analyzing HTTP Request \n");
  char line[1024]={0};
  unsigned int i=0,lines_gathered=0;
  while  (i<request_length)
   {
     if  (request[i]=='\n')
      {
        line[i]=0;

        //We've got ourselves a new line!
        ++lines_gathered;
        AnalyzeHTTPLineRequest(output,line,strlen(line),lines_gathered);
      }
        else
      {
        line[i]=request[i];
      }

     ++i;
   }



  return 0;
}
