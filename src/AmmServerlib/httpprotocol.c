#include "httpprotocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CR 13
#define LF 10




int HTTPRequestComplete(char * request,unsigned int request_length)
{
  if (request_length<2) { return 0; } // at least LF LF is expected :P

  fprintf(stderr,"Checking if request with %u chars is complete .. ",request_length);
  int i=request_length-1;
  while (i>1)
   {
      if ( request[i]==LF )
       {
        if (i>=1)
          {
           if (( request[i-1]==LF )&&( request[i]==LF )) { fprintf(stderr,"it is \n"); return 1; }  // unix 2x new line sequence
          }

         if (i>=3)
          {
           if (( request[i-3]==CR )&&( request[i-2]==LF )&&( request[i-1]==CR )&&( request[i]==LF )) { fprintf(stderr,"it is \n"); return 1; } // windows 2x new line sequence
          }
       }
     --i;
   }

   fprintf(stderr,"it isn't \n");
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
         int s=3;
         while ( (request[s]==' ')&&(s<request_length) ) { ++s; }
         if (s>=request_length) { fprintf(stderr,"Error #1 with GET request\n"); return 0;}
         int e=s;
         while ( (request[e]!=' ')&&(e<request_length) ) { ++e; }
         if (e>=request_length) { fprintf(stderr,"Error #2 with GET request\n"); return 0;}
         request[e]=0; //Signal ending

         char * stripped = &request[s];
         fprintf(stderr,"Stripped GET request is %s \n",stripped);

         output->requestType=GET;
         strncpy(output->resource,stripped,MAX_RESOURCE);

         if (strlen(stripped)>=MAX_RESOURCE) { fprintf(stderr,"Warning : GET request is too big , some bytes dropped..! \n"); }

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


  //Todo keepalive handler here output->keepalive=1;

  return 0;
}

int AnalyzeHTTPRequest(struct HTTPRequest * output,char * request,unsigned int request_length)
{
  fprintf(stderr,"Starting an HTTP Request Analysis\n");
  char line[1024]={0};
  unsigned int i=0,chars_gathered=0,lines_gathered=0;
  while  (i<request_length)
   {
     if  ( ((request[i]==CR)||(request[i]==LF)) && (chars_gathered>0) )
      {
        line[chars_gathered]=0;

        //We've got ourselves a new line!
        ++lines_gathered;
        AnalyzeHTTPLineRequest(output,line,strlen(line),lines_gathered);
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



  return 0;
}
