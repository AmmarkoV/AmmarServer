#include "sendHTTPHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>

#include "file_server.h"

#include "../version.h"
#include "../server_configuration.h"
#include "../tools/logs.h"
#include "../tools/http_tools.h"
#include "../tools/time_provider.h"
#include "../templates/errors.h"

#include "networkAbstraction.h"

unsigned long SendErrorCodeHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction,unsigned int error_code,const char * verified_filename,const char * templates_root)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/
     char response[MAX_HTTP_REQUEST_HEADER_REPLY]={0};
     switch (error_code)
     {
       //TODO switch this to templated.. :
       case 400 : snprintf(response,MAX_HTTP_REQUEST_HEADER_REPLY,"400 Bad Request %s400.html",templates_root);            break;
       case 401 : snprintf(response,MAX_HTTP_REQUEST_HEADER_REPLY,"401 Password Protected %s401.html",templates_root);     break;
       case 404 : snprintf(response,MAX_HTTP_REQUEST_HEADER_REPLY,"404 Not Found %s404.html",templates_root);              break;
       case 408 : snprintf(response,MAX_HTTP_REQUEST_HEADER_REPLY,"408 Timed Out %s408.html",templates_root);              break;
       case 500 : snprintf(response,MAX_HTTP_REQUEST_HEADER_REPLY,"500 Internal Server Error %s500.html",templates_root);  break;
       case 501 : snprintf(response,MAX_HTTP_REQUEST_HEADER_REPLY,"501 Not Implemented %s501.html",templates_root);        break;
       //---------------------------------------------------------------------------------------------------------------------------
       default : snprintf(response,MAX_HTTP_REQUEST_HEADER_REPLY,"500 Internal Server Error %s500.html",templates_root);   break;
     };

     char reply_header[MAX_HTTP_REQUEST_HEADER_REPLY+1]={0};
     snprintf(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"HTTP/1.1 %s\nServer: Ammarserver/%s\nContent-type: text/html\n",response,FULLVERSION_STRING);
     unsigned int replyHeaderLength = strlen(reply_header);

     GetDateString(reply_header+replyHeaderLength, MAX_HTTP_REQUEST_HEADER_REPLY-replyHeaderLength,"Date",1,0,0,0,0,0,0,0);
     int opres=ASRV_Send(instance,transaction,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
     if (opres<=0)
        {
          warning("could not send error code date\n");
          if (verified_filename!=0) { fprintf(stderr,"file we wanted to send was %s ",verified_filename); }

          return 0;
        }

     return 1;
}



unsigned long SendSuccessCodeHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction,int success_code,const char * verified_filename)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/
      char content_type[MAX_CONTENT_TYPE+1]={0};
      strncpy(content_type,"text/html",MAX_CONTENT_TYPE);

      fprintf(stderr,"Sending File %s with response code 200 OK\n",verified_filename);
      GetContentType(verified_filename,content_type,MAX_CONTENT_TYPE);

      char reply_header[MAX_HTTP_REQUEST_HEADER_REPLY+1]={0}; //Accept-Ranges: bytes\n
      snprintf(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"HTTP/1.1 %u OK\nServer: Ammarserver/%s\nContent-type: %s\nCache-Control: max-age=3600\nAccept-Ranges: bytes\n",success_code,FULLVERSION_STRING,content_type);
      unsigned int replyHeaderLength = strlen(reply_header);

      GetDateString(reply_header+replyHeaderLength, MAX_HTTP_REQUEST_HEADER_REPLY-replyHeaderLength,"Date",1,0,0,0,0,0,0,0);
      int opres=ASRV_Send(instance,transaction,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
      if (opres<=0) { fprintf(stderr,"Error sending date\n"); return 0; }

      return 1;
}


unsigned long SendNotModifiedHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/
      char reply_header[MAX_HTTP_REQUEST_HEADER_REPLY+1]={0}; //Accept-Ranges: bytes\n
      snprintf(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"HTTP/1.1 304 Not Modified\nServer: Ammarserver/%s\nCache-Control: max-age=3600\n",FULLVERSION_STRING);
      unsigned int replyHeaderLength = strlen(reply_header);

      GetDateString(reply_header+replyHeaderLength, MAX_HTTP_REQUEST_HEADER_REPLY-replyHeaderLength,"Date",1,0,0,0,0,0,0,0);
      int opres=ASRV_Send(instance,transaction,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
      if (opres<=0) { fprintf(stderr,"Error sending date\n"); return 0; }

      return 1;
}

unsigned long SendAuthorizationHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction,char * message,const char * verified_filename)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/
      char content_type[MAX_CONTENT_TYPE+1]={0};
      strncpy(content_type,"text/html",MAX_CONTENT_TYPE);

      fprintf(stderr,YELLOW "Sending File %s with non-authorized response 401 OK\n" NORMAL ,verified_filename);
      GetContentType(verified_filename,content_type,MAX_CONTENT_TYPE);

      char reply_header[MAX_HTTP_REQUEST_HEADER_REPLY+1]={0}; //Accept-Ranges: bytes\n
      snprintf(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"HTTP/1.1 401 Unauthorized\nServer: Ammarserver/%s\nContent-type: %s\nWWW-Authenticate: Basic realm=\"%s\"\n",FULLVERSION_STRING,content_type,message);
      unsigned int replyHeaderLength = strlen(reply_header);


      GetDateString(reply_header+replyHeaderLength, MAX_HTTP_REQUEST_HEADER_REPLY-replyHeaderLength ,"Date",1,0,0,0,0,0,0,0);
      int opres=ASRV_Send(instance,transaction,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
      if (opres<=0) { fprintf(stderr,"Error sending date\n"); return 0; }

      return 1;
}
