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


#include "version.h"
#include "file_server.h"
#include "file_caching.h"
#include "http_header_analysis.h"
#include "http_tools.h"
#include "configuration.h"
#include "time.h"

/*
   This file contains the main routine called most of the time , i.e. SendFile..!
   the code that follows supposes everything else is ok ( socket / client wise )
   and starts reading and sending the file indicated by the function arguments..!
*/

int files_open;

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
     sprintf(reply_header,"HTTP/1.1 %s\nServer: Ammarserver/%s\nContent-type: text/html\n",response,FULLVERSION_STRING);

     int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL); //Send preliminary header to minimize lag
     if (opres<=0) { return 0; }

     return 1;
}



unsigned long SendSuccessCodeHeader(int clientsock,int success_code,char * verified_filename)
{
/*
    This function serves the first few lines for error headers but NOT all the header and definately NOT the page body..!
    it also changes verified_filename to the appropriate template path for user defined pages for each error code..!
*/
      char content_type[MAX_CONTENT_TYPE+1]={0};
      strncpy(content_type,"text/html",MAX_CONTENT_TYPE);

      fprintf(stderr,"Sending File %s with response code 200 OK\n",verified_filename);
      GetContentType(verified_filename,content_type);

      char reply_header[512]={0}; //Accept-Ranges: bytes\n
      sprintf(reply_header,"HTTP/1.1 %u OK\nServer: Ammarserver/%s\nContent-type: %s\n",success_code,FULLVERSION_STRING,content_type);

      int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL); //Send preliminary header to minimize lag
      if (opres<=0) { return 0; }

      return 1;
}



unsigned long SendFile
  (
    int clientsock, // The socket that will be used to send the data
    char * verified_filename_pending_copy, // The filename to be served on the socket above

    unsigned long start_at_byte,   // Optionally start with an offset ( resume download functionality )
    unsigned long end_at_byte,     // Optionally end at an offset ( resume download functionality )
    unsigned int force_error_code, // Instead of the file , serve an error code..!
    unsigned char header_only,     // Only serve header ( HEAD instead of GET )
    unsigned char keepalive,       // Keep alive functionality
    unsigned char gzip_supported,  // If gzip is supported try to use it!

    //char * webserver_root,
    char * templates_root // In case we fail to serve verified_filename_etc.. serve something from the templates..!
    )
{
  char verified_filename[MAX_FILE_PATH+1]={0};
  char reply_header[MAX_HTTP_RESPONSE_HEADER+1]={0};

  strncpy(verified_filename,verified_filename_pending_copy,MAX_FILE_PATH);


/*!   Start sending the header first..!
      Due to error messages also having body payloads they are also handled here , creating
      clutter in the code but this way there is no need to write the same thing twice..! !*/

/*! PRELIMINARY HEADER SENDING START ----------------------------------------------*/
  if (force_error_code!=0)
  {
    //We want to force a specific error_code!
    if (! SendErrorCodeHeader(clientsock,force_error_code,verified_filename,templates_root) ) { fprintf(stderr,"Failed sending error code %u\n",force_error_code); return 0; }
  } else
  if (!FilenameStripperOk(verified_filename))
  {
     //Unsafe filename , bad request :P
     if (! SendErrorCodeHeader(clientsock,400,verified_filename,templates_root) ) { fprintf(stderr,"Failed sending error code 400\n"); return 0; }
     //verified_filename should now point to the template file for 400 messages
  } else
   {
      //We have a legitimate file to send , if we want to send it all , we must emmit a 200 OK header
      //if we are serving it with an offset , we must emmit a 206 OK header!
      if (start_at_byte!=0)
       {
         //Range Accepted 206 OK header
         if (! SendSuccessCodeHeader(clientsock,206,verified_filename)) { fprintf(stderr,"Failed sending Range Acknowledged success code \n"); return 0; }
       } else
       {
         //Normal 200 OK header
         if (! SendSuccessCodeHeader(clientsock,200,verified_filename)) { fprintf(stderr,"Failed sending success code \n"); return 0; }
       }
   }
/*! PRELIMINARY HEADER SEND END ----------------------------------------------*/



  if (keepalive) { strcat(reply_header,"Connection: keep-alive\n"); } else { strcat(reply_header,"Connection: close\n"); } //Append Keep-Alive or Close and then..

  int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL); //.. send preliminary header to minimize lag
  if (opres<=0) { fprintf(stderr,"Failed while sending header\n"); return 0; }


  unsigned long cached_lSize=0;
  char * cached_buffer = CheckForCachedVersionOfThePage(verified_filename,&cached_lSize,gzip_supported);

  if (cached_buffer!=0) //&&(cached_lSize!=0) its not bad to have a zero size cache item!
   { /*!Serve cached file !*/
     //if (gzip_supported) { strcat(reply_header,"Content-encoding: gzip\n"); } // Cache can serve gzipped files
     sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) cached_lSize);
     opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
     if (opres<=0) { fprintf(stderr,"Error sending cached header \n"); return 0; }
     if (cached_lSize==0) { fprintf(stderr,"Bug(?) detected , zero cache payload\n"); }
     if (!header_only)
      {
       opres=send(clientsock,cached_buffer,cached_lSize,MSG_WAITALL|MSG_NOSIGNAL);  //Send file as soon as we've got it
      }

     if (opres<=0) { fprintf(stderr,"Error sending cached body\n"); return 0; }
     return 1;
   }
     else
  { /*!Serve file by reading it from disk !*/
    if ((cached_buffer==0)&&(cached_lSize==1)) { /*TODO : Cache indicates that file doesn't exist */ } else
    if ((cached_buffer==0)&&(cached_lSize==0)) { /*TODO : Cache indicates that file is not in cache :P */ }


    fprintf(stderr,"fopen(%s,\"rb\") , files open %u \n",verified_filename,files_open);
    FILE * pFile = fopen (verified_filename, "rb" );
    if (pFile==0) { fprintf(stderr,"Could not open file %s\n",verified_filename); return 0;}
    ++files_open;

    // obtain file size:
    if ( fseek (pFile , 0 , SEEK_END)!=0 )
      {
        fprintf(stderr,"Could not find file size..!\nUnable to serve client\n");
        fclose(pFile);
        --files_open;
        return 0;
      }

    unsigned long lSize = ftell (pFile);
    if ( (end_at_byte!=0) && (lSize<end_at_byte) ) { fprintf(stderr,"TODO: Handle  incorrect range request ( from %u to %u file 0 to %u ..!\n",(unsigned int) start_at_byte,(unsigned int) end_at_byte,(unsigned int) lSize); }

    fprintf(stderr,"Sending file %s , size %0.2f Kbytes\n",verified_filename,(double) lSize/1024);

    sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) lSize);

    opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
    if (opres<=0) { fprintf(stderr,"Failed sending content length..!\n"); } else
    if ((unsigned int) opres!=lSize) { fprintf(stderr,"Failed sending the whole content length..!\n"); }


    if (!header_only)
    {
      rewind (pFile);
      if (start_at_byte!=0) { fseek (pFile , start_at_byte , SEEK_SET); }

      //This is the file remaining to be sent..
      unsigned long file_size_remaining = lSize-start_at_byte;
      //We dont want the server to allocate a big enough space to reduce disk reading overheads
      //but we dont want to allocate huge portions of memory so we set a soft limit here
      unsigned long malloc_size  = MAX_FILE_READ_BLOCK_KB;
      //Of course in case that the size to send is smaller than our limit we will commit a smaller amount of memory
      if (file_size_remaining < malloc_size) { malloc_size=file_size_remaining; }

      // allocate memory to contain the whole file:
      //TODO: make a smaller allocation and gradually serve the whole file :P
      char * buffer = (char*) malloc ( sizeof(char) * (malloc_size));

      if (buffer == 0)
        {
          fprintf(stderr," Could not allocate enough memory to serve file %s\n",verified_filename);
          fclose (pFile);
          --files_open;
          return 0;
        }


      while ( file_size_remaining>0 )
      {
        // copy the file into the buffer:
        size_t result;
        result = fread (buffer,1,malloc_size,pFile);

        if (result != malloc_size)
        {
         if (feof(pFile))
          {
             // Reached end of file , cool..!
          }   else
          {
             fprintf(stderr,"Reading error %u while reading file %s",ferror(pFile),verified_filename);
             free (buffer);
             fclose (pFile);
             --files_open;
             return 0;
          }
        }

      //A timer added partly as vanity code , partly to get transmission speeds for qos ( later on )
      struct time_snap time_to_serve_file_s;
      start_timer (&time_to_serve_file_s);
       //ACTUAL SENDING OF FILE -->
        opres=send(clientsock,buffer,result,MSG_WAITALL|MSG_NOSIGNAL);  //Send file as soon as we've got it
        /* the whole file should now have reached our client .! */
        if (opres<=0) { fprintf(stderr,"Connection closed , while sending the whole file..!\n"); file_size_remaining=0; } else
        {
          if ((unsigned int) opres!=result) { fprintf(stderr,"TODO : Handle , failed sending the whole file..!\n"); }
          file_size_remaining-=opres;
        }



       //ACTUAL SENDING OF FILE <--
      double time_to_serve_file = (double ) end_timer (&time_to_serve_file_s) / 1000000; // go to seconds
      double speed_in_Mbps= 0;
      if (time_to_serve_file>0)
       {
        speed_in_Mbps = (double ) opres/1048576;
        speed_in_Mbps = (double ) speed_in_Mbps/time_to_serve_file;
        fprintf(stderr,"Current transmission rate = %0.2f Mbytes/sec , in %0.5f seconds\n",speed_in_Mbps,time_to_serve_file);
       }
      //End of timer code



      }


      // terminate
      free (buffer);
  }

  fprintf(stderr,"Closing file handler for %s ( files open %u )\n",verified_filename,files_open);
  --files_open;
  fclose (pFile);

  return 1;
  }

 return 0;
}





unsigned long SendFileMemory
  (
    int clientsock, // The socket that will be used to send the data
    //char * path, // The filename to be served on the socket above
    char * mem, // The memory block body to be sent
    unsigned long mem_block // The size of the memory block to be sent
  )
{
  char reply_header[MAX_HTTP_RESPONSE_HEADER+1]={0};
  if (! SendSuccessCodeHeader(clientsock,200,"dir.html")) { fprintf(stderr,"Failed sending success code \n"); return 0; }
  sprintf(reply_header,"Content-length: %u\n",(unsigned int) mem_block);
  strcat(reply_header,"Connection: close\n\n");
  //TODO : Location : path etc
  int opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
  if (opres<=0) { fprintf(stderr,"Failed sending content length..!\n"); } else
  if ((unsigned int) opres!=mem_block) { fprintf(stderr,"Failed sending the whole content length..!\n"); }


  opres=send(clientsock,mem,mem_block,MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
  if (opres<=0) { fprintf(stderr,"Failed sending content length..!\n"); } else
  if ((unsigned int) opres!=mem_block) { fprintf(stderr,"Failed sending the whole content length..!\n"); }

 return 0;
}
