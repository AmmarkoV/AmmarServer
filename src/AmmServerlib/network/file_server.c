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

#include <sys/stat.h>
#include <time.h>

#include "../version.h"
#include "file_server.h"
#include "../cache/file_caching.h"
#include "../header_analysis/http_header_analysis.h"
#include "../server_configuration.h"
#include "../tools/http_tools.h"
#include "../tools/time_provider.h"
#include "../tools/logs.h"

#include "sendHTTPHeader.h"

/*
   This file contains the main routine called most of the time , i.e. SendFile..!
   the code that follows supposes everything else is ok ( socket / client wise )
   and starts reading and sending the file indicated by the function arguments..!
*/

unsigned int files_open = 0;


int SendPart(int clientsock,char * message,unsigned int message_size)
{
  int opres=send(clientsock,message,message_size,MSG_WAITALL|MSG_NOSIGNAL);
  if (opres<=0) { fprintf(stderr,"Failed sending `%s`..!\n",message); return 0; } else
  if ((unsigned int) opres!=message_size) { fprintf(stderr,"Failed sending the whole message (%s)..!\n",message); return 0; }
  return 1;
}


inline int TransmitFileToSocketInternal(
                                         FILE * pFile,
                                         int clientsock,
                                         unsigned long bytesToSendStart
                                       )
{
    //We dont want the server to allocate a big enough space to reduce disk reading overheads
    //but we dont want to allocate huge portions of memory so we set a soft limit here
    unsigned long bytesToSend = bytesToSendStart;
    unsigned long malloc_size  = MAX_FILE_READ_BLOCK_KB;
    //Of course in case that the size to send is smaller than our limit we will commit a smaller amount of memory
    if (bytesToSend < malloc_size) { malloc_size=bytesToSend; }

    // allocate memory to contain the whole file:
    size_t chunkToSend;
    char * buffer = (char*) malloc ( sizeof(char) * (malloc_size));
    char * rollingBuffer = buffer;
    if (buffer == 0)
        { error(" Could not allocate enough memory to serve file %s\n"); return 0; }


      #if TIME_UPLOADS
      //A timer added partly as vanity code , partly to get transmission speeds for qos ( later on )
      struct time_snap time_to_serve_file_s;
      start_timer (&time_to_serve_file_s);
      #endif

      int opres=1; // <- This needs to be 1 so that the initial while statement won't fail
      while ( ( bytesToSend>0 ) && ( opres >=0 ) )
      {
        // copy the file into the buffer:
        chunkToSend = fread (buffer,1,malloc_size,pFile);

        if (chunkToSend != malloc_size)
        {
          if (!feof(pFile))
          {  //If we reached the end of the file and thats why we read fewer bytes , and it is ok
             //If we haven't reached the end of the file then we have encountered a read error!
             fprintf(stderr,"Reading error %u while reading file \n",ferror(pFile));
             free (buffer);
             return 0;
          }
        }

        rollingBuffer = buffer;
        opres=1; // <- This needs to be 1 so that the initial while statement won't fail
        while ( (chunkToSend>0) && (opres>=0) )
        {
           opres=send(clientsock,rollingBuffer,chunkToSend,MSG_WAITALL|MSG_NOSIGNAL);  //Send file parts as soon as we've got them
           if (opres == 0) {  /*Recepient stalling */ } else
           if (opres < 0) { warning("Connection closed , while sending the whole file..!\n"); }
                          {
                           chunkToSend -= opres;
                           bytesToSend -= opres;
                           rollingBuffer += opres;
                          }
           //fprintf(stderr,".");
        }
      } // End of having a remaining file to send


      #if TIME_UPLOADS
      double time_to_serve_file_in_seconds = (double ) end_timer(&time_to_serve_file_s) / 1000000; // go to seconds
      double speed_in_Mbps= 0;
      if (time_to_serve_file_in_seconds>0)
       {
        speed_in_Mbps = ( double ) ( (bytesToSendStart-bytesToSend) /*Bytes Sent*/  /1048576);
        speed_in_Mbps = ( double ) speed_in_Mbps/time_to_serve_file_in_seconds;
        fprintf(stderr,"Current transmission rate = %0.2f Mbytes/sec , in %0.5f seconds\n",speed_in_Mbps,time_to_serve_file_in_seconds);
       } else
       { fprintf(stderr,"Spontaneous transmission(!)\n"); }
      //End of timer code
      #endif

      // terminate
      free (buffer);
      return 1;
}









int TransmitFileToSocket(
                            int clientsock,
                            char * verified_filename,
                            unsigned long start_at_byte,   // Optionally start with an offset ( resume download functionality )
                            unsigned long end_at_byte     // Optionally end at an offset ( resume download functionality )
                         )
{
    FILE * pFile = fopen (verified_filename, "rb" );
    if (pFile==0) { fprintf(stderr,"Could not open file %s , files open %u \n",verified_filename,files_open); return 0;}
    ++files_open;

    // obtain file size:
    if ( fseek (pFile , 0 , SEEK_END)!=0 )
      {
        warning("Could not find file size..!\nUnable to serve client\n");
        fclose(pFile);
        --files_open;
        return 0;
      }

    unsigned long lSize = ftell (pFile);
    if ( (end_at_byte!=0) && (lSize<end_at_byte) )
        { fprintf(stderr,"TODO: Handle  incorrect range request ( from %u to %u file 0 to %u ..!\n",(unsigned int) start_at_byte,(unsigned int) end_at_byte,(unsigned int) lSize); }

    fprintf(stderr,"Sending file %s , size %0.2f Kbytes , Open files %u \n",verified_filename,(double) lSize/1024,files_open);

    char reply_header[512]={0};
    //THIS ALSO EXISTS IN THE Cached resource response CODE around line 395
    if ( (start_at_byte!=0) || (end_at_byte!=0) )
       {
         //error(" TransmitFileToSocket Content-Range response");
         //Content-Range: bytes 1000-3979/3980
         int endAtBytePrinted = end_at_byte;
         if (endAtBytePrinted == 0 ) { endAtBytePrinted = lSize; }
          sprintf(reply_header,"Content-Range: bytes %u-%u/%u\nContent-length: %u\n\n",start_at_byte,endAtBytePrinted,lSize,lSize-start_at_byte);
       } else
       {
         //error("TransmitFileToSocket Plain Content-Length ");
         //This is the last header part , so we are appending an extra \n to mark the end of the header
         sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) lSize);
       }
    if (!SendPart(clientsock,reply_header,strlen(reply_header))) { fprintf(stderr,"Failed sending Content-length @  SendFile ..!\n");  }


    rewind (pFile);
    if (start_at_byte!=0) { fseek (pFile , start_at_byte , SEEK_SET); }
    if (end_at_byte==0)   { end_at_byte=lSize-start_at_byte; }


    int res = TransmitFileToSocketInternal(pFile,clientsock,end_at_byte);

    --files_open;
    fclose (pFile);
  return res;
}



unsigned long SendFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    char * verified_filename_pending_copy, // The filename to be served on the socket above
    unsigned int force_error_code
  )
{
 int clientsock = transaction->clientSock;
 unsigned int resourceCacheID=transaction->resourceCacheID;
 unsigned long start_at_byte = transaction->incomingHeader.range_start;
 unsigned long end_at_byte = transaction->incomingHeader.range_end;
 unsigned char keepalive = transaction->incomingHeader.keepalive;
 unsigned char compression_supported = transaction->incomingHeader.supports_compression ;


  struct HTTPHeader * request = &transaction->incomingHeader;

  char verified_filename[MAX_FILE_PATH+1]={0};
  char reply_header[MAX_HTTP_RESPONSE_HEADER+1]={0};

  if (verified_filename_pending_copy != 0)
  {
     strncpy(verified_filename,verified_filename_pending_copy,MAX_FILE_PATH);
  }


/*!   Start sending the header first..!
      Due to error messages also having body payloads they are also handled here , creating
      clutter in the code but this way there is no need to write the same thing twice..! !*/

/*! PRELIMINARY HEADER SENDING START ----------------------------------------------*/
  unsigned int WeWantA200OK=0;

  if (force_error_code!=0)
  {
    //We want to force a specific error_code!
    if (! SendErrorCodeHeader(clientsock,force_error_code,verified_filename,instance->templates_root) ) { fprintf(stderr,"Failed sending error code %u\n",force_error_code); return 0; }
  } else
  if (!FilenameStripperOk(verified_filename))
  {
     //Unsafe filename , bad request :P
     if (! SendErrorCodeHeader(clientsock,400,verified_filename,instance->templates_root) ) { fprintf(stderr,"Failed sending error code 400\n"); return 0; }
     //verified_filename should now point to the template file for 400 messages
  } else
   {
      //We have a legitimate file to send , if we want to send it all , we must emmit a 200 OK header
      //if we are serving it with an offset , we must emmit a 206 OK header!
      if ( (start_at_byte!=0) || (end_at_byte!=0) )
       {
         error("No checking on Range Provided is done , the underlying mechanisms are safe , but the header could potentially display wrong things ..");
         error("We dont know the filesize yet so can't fix it here..");

         //Range Accepted 206 OK header
         if (! SendSuccessCodeHeader(clientsock,206,verified_filename)) { fprintf(stderr,"Failed sending Range Acknowledged success code \n"); return 0; }
       } else
       {
         //Normal 200 OK header
         /*! TODO Reorganize this : THIS SHOULD NOT BE SENT YET , SINCE WE MAY WANT TO EMMIT A 304 Not Modified Header if content is unmodified..!*/
         WeWantA200OK=1;
       }
   }
/*! PRELIMINARY HEADER SEND END ----------------------------------------------*/


  struct cache_item * cache = (struct cache_item *) instance->cache;

  int opres=0;
  unsigned int index=0;
  unsigned long cached_lSize=0;
  unsigned char cached_buffer_is_compressed = compression_supported;
  unsigned char free_cached_buffer_after_use=0;
  char * cached_buffer = cache_GetResource(instance,request,resourceCacheID,verified_filename,&index,&cached_lSize,0,&cached_buffer_is_compressed,&free_cached_buffer_after_use);

  if  (cached_buffer!=0) //If we have already a cached version of the file there is a change we might send a 304 Not Modified response
   {
      unsigned char ok_to_serve_not_modified = 1;

      /*OK We have a cached buffer on this page BUT a good question to ask is the following..
        Is it a regular file we are talking about , or a dynamic page ? */

      if (cache[index].dynamicRequestCallbackFunction!=0)
              {
                /*It seems we have ourselves a dynamic page..! Are we on a callback cooldown ? */
                /*The only built in way to serve a not modified response is if the request is too soon ( callback_every_x_msec callback cooldown ) ! */
                struct AmmServer_RH_Context * shared_context = cache[index].dynamicRequest;
                if ( shared_context->callback_cooldown ) { ok_to_serve_not_modified=1; } else // <- the dynamic page is still fresh.. so lets serve not modified..!
                                                         { ok_to_serve_not_modified=0; } // <- the dynamic page has expired is dynamic so it can't be cached
              } else
              {
                  //It seems we have ourselves a regular page
                  //ok_to_serve_not_modified already should equal 1 so leave this here as documentation.. :P
              }

      //The application might want the file to always be served as a fresh one..
      if ( cache[index].doNOTCacheRule ) { ok_to_serve_not_modified = 0; } /*We have written orders that we want this file to NEVER get cached.. EVER :P */
      if (force_error_code!=0) { ok_to_serve_not_modified = 0; } /*We want 404 etc messages to remain 404 :P , no point in serving 404 and then 304 ( that the 404 didn't change )*/

      if (ok_to_serve_not_modified)
      {
       //Check E-Tag here..!
       unsigned int cache_etag = cache_GetHashOfResource(instance,index);
       if ((request->eTag!=0)&&(cache_etag!=0))
        {
          fprintf(stderr,"E-Tag is `%s` , local hash is `%u` \n",request->eTag,cache_etag);
          char LocalETag[40]={0};
          sprintf(LocalETag,"\"%u\"",cache_etag);
          if ( strncmp(request->eTag,LocalETag,request->eTagLength)==0 )
           {
              fprintf(stderr,"The content matches our ETag , we will reply with 304 NOT MODIFIED! :) \n");
              SendNotModifiedHeader(clientsock);

              //The Etag is mandatory on 304 messages..!
              char ETagSendChunk[128]={0};
              sprintf(ETagSendChunk,"ETag: \"%u%u%u\" \n",cache_etag,start_at_byte,end_at_byte);
              if (!SendPart(clientsock,ETagSendChunk,strlen(ETagSendChunk))) { fprintf(stderr,"Failed sending content length @  SendMemoryBlockAsFile ..!\n");  }

              WeWantA200OK=0;
              request->requestType=HEAD;
           }
             else
          {
            warning("eTag Mismatch\n"); // <- for now mismatches are probably bugs
          }
        }
     }
   }

   unsigned int have_last_modified=0;
   struct stat last_modified;

   if ( WeWantA200OK )
   {
       if (! SendSuccessCodeHeader(clientsock,200,verified_filename)) { fprintf(stderr,"Failed sending success code \n"); freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }

       /* TODO : TEMPORARILY DISABLED LAST-MODIFIED :P
       if (stat(verified_filename, &last_modified))  { fprintf(stderr,"Could not stat modification time for file %s\n",verified_filename); } else
                                                     {  have_last_modified=1; }*/

       //TODO -> Check with last modified -> char * cached_buffer = CheckForCachedVersionOfThePage(request,verified_filename,&index,&cached_lSize,0,gzip_supported);
   }

   if (have_last_modified)
     {

       struct tm * ptm = gmtime ( &last_modified.st_mtime ); //This is not a particularly thread safe call , must add a mutex or something here..!
       //Last-Modified: Sat, 29 May 2010 12:31:35 GMT
       GetDateString(reply_header,"Last-Modified",0,ptm->tm_wday,ptm->tm_mday,ptm->tm_mon,EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
       opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
       if (opres<=0) { fprintf(stderr,"Error sending Last-Modified header \n"); freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }
     }
                 //This used to also emmit --> Keep-Alive: timeout=5, max=100\n <--
                 /* RedBot says ( http://redbot.org/?uri=http%3A%2F%2Fammar.gr%3A8080%2F ) ..!
                     The Keep-Alive header is completely optional; it is defined primarily because the keep-alive connection token implies that such a header exists, not because anyone actually uses it.
                    Some implementations (e.g., Apache) do generate a Keep-Alive header to convey how many requests they're willing to serve on a single connection, what the connection timeout is and other information. However, this isn't usually used by clients.
                    It's safe to remove this header if you wish to save a few bytes in the response.*/
  if (keepalive) { if (!SendPart(clientsock,"Connection: keep-alive\n",strlen("Connection: keep-alive\n")) ) { /*TODO : HANDLE failure to send Connection: Keep-Alive */}  } else
                 { if (!SendPart(clientsock,"Connection: close\n",strlen("Connection: close\n"))) { /*TODO : HANDLE failure to send Connection: Close */}  }


if (request->requestType!=HEAD)
 {
  if ( (cached_buffer!=0) && //If we haven't got a buffer cached.. AND
        ( (!cache[index].doNOTCacheRule) /*If we dont forbid caching */ || ( (cache[index].doNOTCacheRule)&&(cache[index].dynamicRequestCallbackFunction!=0) ) /*Or we forbid caching but we are talking about a dynamic page*/)
     ) // its ok to serve a cached file..
   { /*!Serve cached file !*/
     //if (gzip_supported) { strcat(reply_header,"Content-encoding: gzip\n"); } // Cache can serve gzipped files
     //Last-Modified: Sat, 29 May 2010 12:31:35 GMT
     //GetDateString(reply_header,"Date",1,0,0,0,0,0,0,0);


     unsigned int  cache_etag = cache_GetHashOfResource(instance,index);
     if (cache_etag!=0)
     {
        sprintf(reply_header,"ETag: \"%u%u%u\"\n",cache_etag,start_at_byte,end_at_byte);
        opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send E-Tag as soon as we've got it
        if (opres<=0) { fprintf(stderr,"Error sending ETag header \n"); freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }

     }

     if (cached_lSize==0) { warning("Bug(?) detected , zero cache payload\n"); }


     if ( cached_buffer_is_compressed )
     {
        strcpy(reply_header,"Content-Encoding: deflate\n");
        opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send E-Tag as soon as we've got it
        if (opres<=0) { fprintf(stderr,"Error sending Compression header \n"); freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }
     }

    //THIS ALSO EXISTS IN THE TransmitFileToSocket  CODE
    //Send Content Length , as a range , or as the whole file!
      if ( (start_at_byte!=0) || (end_at_byte!=0) )
       {
         //error("Resource Content-Range response");
         //Content-Range: bytes 1000-3979/3980
         int endAtBytePrinted = end_at_byte;
         if (endAtBytePrinted == 0 ) { endAtBytePrinted = cached_lSize; }
          sprintf(reply_header,"Content-Range: bytes %u-%u/%u\nContent-length: %u\n\n",start_at_byte,endAtBytePrinted,cached_lSize,cached_lSize-start_at_byte);
       } else
       {
         //error("Resource Plain Content-Length ");
         //This is the last header part , so we are appending an extra \n to mark the end of the header
         sprintf(reply_header,"Content-length: %u\n\n",(unsigned int) cached_lSize);
       }
     opres=send(clientsock,reply_header,strlen(reply_header),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
     if (opres<=0) { fprintf(stderr,"Error sending cached header \n"); freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }

     opres=send(clientsock,cached_buffer,cached_lSize,MSG_WAITALL|MSG_NOSIGNAL);  //Send file as soon as we've got it
     freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);

     if (opres<=0) { fprintf(stderr,"Error sending cached body\n"); return 0; }
     return 1;
   }
     else
  {
    /*!Serve file by reading it from disk !*/

    if ((cached_buffer==0)&&(cached_lSize==1)) { /*TODO : Cache indicates that file doesn't exist */ } else
    if ((cached_buffer==0)&&(cached_lSize==0)) { /*TODO : Cache indicates that file is not in cache :P */ }



     if ( !TransmitFileToSocket(clientsock,verified_filename,start_at_byte,end_at_byte) )
      {
         fprintf(stderr,"Could not transmit file %s \n",verified_filename);
         return 0;
      }
      return 1;
  }
  //
} //we also want a body with that header END
 else
{
  //We only served a header so lets append the last new line char..!
  send(clientsock,"\n",strlen("\n"),MSG_WAITALL|MSG_NOSIGNAL);
}


 return 0;
}



unsigned long SendErrorFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    unsigned int errorCode
  )
{
  return SendFile(instance,transaction,0,errorCode);
}




unsigned long SendMemoryBlockAsFile
  (
    char * filename,
    int clientsock, // The socket that will be used to send the data
    //char * path, // The filename to be served on the socket above
    char * mem, // The memory block body to be sent
    unsigned long mem_block // The size of the memory block to be sent
  )
{
  char reply_header[MAX_HTTP_RESPONSE_HEADER+1]={0};
  if (! SendSuccessCodeHeader(clientsock,200,filename)) { fprintf(stderr,"Failed sending success code \n"); return 0; }
  sprintf(reply_header,"Content-length: %u\n",(unsigned int) mem_block);
  strcat(reply_header,"Connection: close\n\n");
  //TODO : Location : path etc

  if (!SendPart(clientsock,reply_header,strlen(reply_header)))
      { fprintf(stderr,"Failed sending content length @  SendMemoryBlockAsFile ..!\n");  }

  if (!SendPart(clientsock,mem,mem_block))
      { fprintf(stderr,"Failed sending content body @  SendMemoryBlockAsFile ..!\n");  }

 return 0;
}
