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
#include "file_caching.h"
#include "httpprotocol.h"
#include "httprules.h"
#include "configuration.h"

/*
   This file contains the main routine called most of the time , i.e. SendFile..!
   the code that follows supposes everything else is ok ( socket / client wise )
   and starts reading and sending the file indicated by the function arguments..!
*/



unsigned long SendErrorCodeHeader(int clientsock,unsigned int error_code,char * verified_filename,char * templates_root)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/

     char response[512]={0};
     switch (error_code)
     {
       case 400 : strcpy(response,"400 Bad Request");            strcpy(verified_filename,templates_root); strcat(verified_filename,"400.html"); break;
       case 401 : strcpy(response,"401 Password Protected");     strcpy(verified_filename,templates_root); strcat(verified_filename,"401.html"); break;
       case 404 : strcpy(response,"404 Not Found");              strcpy(verified_filename,templates_root); strcat(verified_filename,"404.html"); break;
       case 408 : strcpy(response,"408 Timed Out");              strcpy(verified_filename,templates_root); strcat(verified_filename,"408.html"); break;
       case 500 : strcpy(response,"500 Internal Server Error");  strcpy(verified_filename,templates_root); strcat(verified_filename,"500.html"); break;
       case 501 : strcpy(response,"501 Not Implemented");        strcpy(verified_filename,templates_root); strcat(verified_filename,"501.html"); break;
       //---------------------------------------------------------------------------------------------------------------------------
       default : strcpy(response,"500 Internal Server Error");  strcpy(verified_filename,templates_root); strcat(verified_filename,"500.html"); break;
     };


     char reply_header[512]={0};

     sprintf(reply_header,"HTTP/1.1 %s\nServer: Ammarserver/%s\nContent-type: text/html\n",response,AmmServerVERSION);

     int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL); //Send preliminary header to minimize lag
     if (opres<=0) { return 0; }

     return 1;
}






unsigned long SendFile
  (
    int clientsock, // The socket that will be used to send the data
    char * verified_filename_pending_copy, // The filename to be served on the socket above

    unsigned long start_at_byte,   // Optionally start with an offset ( resume download functionality )
    unsigned int force_error_code, // Instead of the file , serve an error code..!
    unsigned char header_only,     // Only serve header ( HEAD instead of GET )
    unsigned char keepalive,       // Keep alive functionality
    unsigned char gzip_supported,  // If gzip is supported try to use it!

    //char * webserver_root,
    char * templates_root // In case we fail to serve verified_filename_etc.. serve something from the templates..!
    )
{
  char verified_filename[MAX_FILE_PATH]={0};
  char reply_header[MAX_HTTP_RESPONSE_HEADER]={0};
  char content_type[MAX_CONTENT_TYPE]={0};

  strncpy(verified_filename,verified_filename_pending_copy,MAX_FILE_PATH);
  strncpy(content_type,"text/html",MAX_CONTENT_TYPE);


/*!   Start sending the header first..!
      Due to error messages also having body payloads they are also handled here , creating
      clutter in the code but this way there is no need to write the same thing twice..! !*/

/*! PRELIMINARY HEADER SEND START ----------------------------------------------*/
  if (force_error_code!=0)
  {
    //We want to force a specific error_code!
    SendErrorCodeHeader(clientsock,force_error_code,verified_filename,templates_root);
  } else
  if (!FilenameStripperOk(verified_filename))
  {
     //Unsafe filename , bad request :P
     SendErrorCodeHeader(clientsock,400,verified_filename,templates_root);
     //verified_filename should now point to the template file for 400 messages
  } else
  if (!FileExists(verified_filename))
   {
     //File doesnt exist , 404 error :P
     fprintf(stderr,"File Requested (%s) does not exist \n",verified_filename);
     SendErrorCodeHeader(clientsock,404,verified_filename,templates_root); //verified_filename should now point to the template file for 404 messages
   } else
   {
      //Normal 200 OK header
      fprintf(stderr,"Sending File %s with response code 200 OK\n",verified_filename);
      GetContentType(verified_filename,content_type);
      sprintf(reply_header,"HTTP/1.1 200 OK\nServer: Ammarserver/%s\nContent-type: %s\n",AmmServerVERSION,content_type);
   }
/*! PRELIMINARY HEADER SEND END ----------------------------------------------*/



  if (keepalive) { strcat(reply_header,"Connection: keep-alive\n"); } else { strcat(reply_header,"Connection: close\n"); } //Append Keep-Alive or Close and then..

  int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL); //.. send preliminary header to minimize lag
  if (opres<=0) { fprintf(stderr,"Failed while sending header\n"); return 0; }


  unsigned long cached_lSize=0;
  char * cached_buffer = CheckForCachedVersionOfThePage(verified_filename,&cached_lSize,gzip_supported);

  if ((cached_buffer!=0)&&(cached_lSize!=0))
   { /*!Serve cached file !*/
     if (gzip_supported) { strcat(reply_header,"Content-encoding: gzip\n"); } // Cache can serve gzipped files
     sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) cached_lSize);
     opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);  //Send filesize as soon as we've got it
     if (!header_only)
      {
       opres=send(clientsock,cached_buffer,cached_lSize,MSG_WAITALL);  //Send file as soon as we've got it
      }
     return opres;
   }
     else
  { /*!Serve file by reading it from disk !*/
    if ((cached_buffer==0)&&(cached_lSize==1)) { /*TODO : Cache indicates that file doesn't exist */ } else
    if ((cached_buffer==0)&&(cached_lSize==0)) { /*TODO : Cache indicates that file is not in cache :P */ }


    fprintf(stderr,"fopen(%s,\"rb\")\n",verified_filename);
    FILE * pFile = fopen (verified_filename, "rb" );
    if (pFile==0) { fprintf(stderr,"Could not open file %s\n",verified_filename); return 0;}


    fprintf(stderr,"Sending file %s\n",verified_filename);
    // obtain file size:
    fseek (pFile , 0 , SEEK_END);

    unsigned long lSize = ftell (pFile);
    sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) lSize);
    opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL);  //Send filesize as soon as we've got it


    if (!header_only)
    {
      rewind (pFile);
      if (start_at_byte!=0) { fseek (pFile , start_at_byte , SEEK_SET); }

      // allocate memory to contain the whole file:
      //TODO: make a smaller allocation and gradually serve the whole file :P
      char * buffer = (char*) malloc ( sizeof(char) * (lSize));

      if (buffer == 0)
        {
          fprintf(stderr," Could not allocate enough memory to serve file %s\n",verified_filename);
          fclose (pFile);
          return 0;
        }

      // copy the file into the buffer:
      size_t result;
      result = fread (buffer,1,lSize,pFile);

      if (result != lSize)
       {
         fputs ("Reading error",stderr);
         free (buffer);
         fclose (pFile);
         return 0;
        }


      opres=send(clientsock,buffer,result,MSG_WAITALL);  //Send file as soon as we've got it
      /* the whole file is now loaded in the memory buffer. */
      if (opres<=0) { fprintf(stderr,"Failed sending file..!\n"); } else
      if ((unsigned int) opres!=lSize) { fprintf(stderr,"Failed sending the whole file..!\n"); }


      // terminate
      free (buffer);
  }

  fprintf(stderr,"Closing file handler for %s\n",verified_filename);
  fclose (pFile);

  return 1;
  }

 return 0;
}
