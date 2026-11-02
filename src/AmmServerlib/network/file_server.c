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
// --------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
// --------------------------------------------
#include <sys/stat.h>
#include <time.h>
// --------------------------------------------
#include "../version.h"
#include "file_server.h"
// --------------------------------------------
#include "../cache/file_caching.h"
#include "../header_analysis/http_header_analysis.h"
#include "../server_configuration.h"
#include "../tools/http_tools.h"
#include "../tools/time_provider.h"
#include "../tools/logs.h"
// --------------------------------------------
#include "../templates/errors.h"
#include "../templates/icons.h"
// --------------------------------------------
#include "sendHTTPHeader.h"
// --------------------------------------------
#include "networkAbstraction.h"

/*
   This file contains the main routine called most of the time , i.e. SendFile..!
   the code that follows supposes everything else is ok ( socket / client wise )
   and starts reading and sending the file indicated by the function arguments..!
*/

int SendPart(
              struct AmmServer_Instance * instance,
              struct HTTPTransaction * transaction,
              const char * message,
              unsigned int message_size,
              int more_data_follows /* 1 if another send for this same response follows immediately - lets the
                                        kernel coalesce this with the next write into fewer TCP segments */
             )
{
  int flags = MSG_WAITALL|MSG_NOSIGNAL;
  if (more_data_follows) { flags |= MSG_MORE; }
  int opres=ASRV_Send(instance,transaction,message,message_size,flags);
  if (opres<=0)
     {
      fprintf(stderr,"Failed to SendPart `%s`..!\n",message);
      return 0;
     } else
  if ((unsigned int) opres!=message_size)
     {
      //TODO : send the rest of it maybe?
      fprintf(stderr,"Failed SendPart to send the whole message (%s)..!\n",message);
      return 0;
     }
     //else
    // {
       //Always positive
    //   instance->statistics.totalUploadKB+=(unsigned long) opres/1024;
    // }
  return 1;
}

 int TransmitFileToSocketInternal(
                                   struct AmmServer_Instance * instance,
                                   FILE * pFile,
                                   struct HTTPTransaction * transaction,
                                   unsigned long bytesToSendStart
                                  )
{
    //We dont want the server to allocate a big enough space to reduce disk reading overheads
    //but we dont want to allocate huge portions of memory so we set a soft limit here
    unsigned long bytesToSend = bytesToSendStart;
    if (bytesToSend==0) { return 1; } //Nothing to transmit ( e.g. a zero length range slice )
    unsigned long malloc_size  = MAX_FILE_READ_BLOCK_KB*1024; // 17-09-15 <- I had forgotten the *1024 This was very small and caused weird high CPU loads that took me 2 days to debug
    //Of course in case that the size to send is smaller than our limit we will commit a smaller amount of memory
    if (bytesToSend < malloc_size) { malloc_size=bytesToSend; }

    // allocate memory to contain the whole file:
    size_t chunkToSend;
    char * buffer = (char*) malloc ( sizeof(char) * (malloc_size));
    char * rollingBuffer = buffer;
    if (buffer == 0)
        {
          errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY);
          return 0;
        }


      #if CALCULATE_TIME_FOR_UPLOADS
      //A timer added partly as vanity code , partly to get transmission speeds for qos ( later on )
      struct time_snap time_to_serve_file_s;
      startTimer (&time_to_serve_file_s);
      #endif

      int stopFileTransmission=0;
      int opres=1; // <- This needs to be 1 so that the initial while statement won't fail
      while ( ( bytesToSend>0 ) && ( opres >=0 ) && (!stopFileTransmission) )
      {
        // copy the file into the buffer:
        chunkToSend = fread (buffer,1,malloc_size,pFile);

        if (chunkToSend == 0)
          { //The file ended earlier than its advertised size ( it was truncated while serving ) ,
            //return instead of spinning forever in this outer loop.
            AmmServer_Warning("File ended before the expected number of bytes was transmitted\n");
            free (buffer);
            return 0;
          }

        if (chunkToSend != malloc_size)
        {
          if (!feof(pFile))
          {  //If we reached the end of the file and thats why we read fewer bytes , and it is ok
             //If we haven't reached the end of the file then we have encountered a read error!
             fprintf(stderr,"Reading error %u while reading file \n",ferror(pFile));
             perror ("The following error occured : ");
             free (buffer);
             return 0;
          }
        }


        unsigned int currentStalling=0;
        rollingBuffer = buffer;
        opres=1; // <- This needs to be 1 so that the initial while statement won't fail
        while ( (chunkToSend>0) && (opres>=0) && (!stopFileTransmission) )
        {
           opres=ASRV_Send(instance,transaction,rollingBuffer,chunkToSend,MSG_WAITALL|MSG_NOSIGNAL);  //Send file parts as soon as we've got them
           if (opres < 0) {
                            warningID(ASV_WARNING_CONNECTION_CLOSED_WHILE_TRANSMITTING_FILE);
                            stopFileTransmission=1;
                          } else
           if (opres == 0)
                          {
                            /*Recepient stalling */
                            ++currentStalling;
                            if (currentStalling>=MAX_TRANSMISSION_STALL)
                                {
                                  warningID(ASV_WARNING_CONNECTION_MAX_TRANSMISSION_STALL);
                                  stopFileTransmission=1;
                                }
                            fprintf(stderr,".");
                          } else
                          {
                           //instance->statistics.totalUploadKB+=(unsigned long) chunkToSend/1024;
                           chunkToSend -= opres;
                           bytesToSend -= opres;
                           rollingBuffer += opres;
                          }

        }
      } // End of having a remaining file to send

      #if CALCULATE_TIME_FOR_UPLOADS
      double time_to_serve_file_in_seconds = (double ) endTimer(&time_to_serve_file_s) / 1000000; // go to seconds
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
      if (buffer!=0)
         { free (buffer); }
      return 1;
}

int TransmitFileHeaderToSocket(
                                      struct AmmServer_Instance * instance,
                                      struct HTTPTransaction * transaction,
                                      unsigned long start_at_byte,   // Optionally start with an offset ( resume download functionality )
                                      unsigned long end_at_byte,     // Optionally end at an offset ( resume download functionality )
                                      unsigned long lSize
                                     )
{
    char reply_header[MAX_HTTP_REQUEST_SHORT_HEADER_REPLY+1]={0};
    //THIS ALSO EXISTS IN THE Cached resource response CODE around line 395
    if ( (start_at_byte!=0) || (end_at_byte!=0) )
       {
         unsigned long endAtBytePrinted = end_at_byte;
         if (endAtBytePrinted == 0 )
              { endAtBytePrinted = lSize; }

          snprintf(
                    reply_header,
                    MAX_HTTP_REQUEST_SHORT_HEADER_REPLY,
                    "Content-Range: bytes %lu-%lu/%lu\nContent-Length: %lu\r\n\r\n",
                    start_at_byte,
                    endAtBytePrinted,
                    lSize,
                    lSize-start_at_byte
                   );
       } else
       {
         //This is the last header part , so we are appending an extra \n to mark the end of the header
         snprintf(
                   reply_header,
                   MAX_HTTP_REQUEST_SHORT_HEADER_REPLY,
                   "Content-Length: %lu\r\n\r\n",
                   lSize
                 );
       }
    if (!SendPart(instance,transaction,reply_header,strlen(reply_header),1)) //the file body always follows this
        {
         fprintf(stderr,"Failed sending Content-Length @  SendFile ..!\n");
         return 0;
        }
  return 1;
}

int TransmitFileToSocket(
                         struct AmmServer_Instance * instance,
                         struct HTTPTransaction * transaction,
                         const char * verified_filename,
                         unsigned long start_at_byte,   // Optionally start with an offset ( resume download functionality )
                         unsigned long end_at_byte     // Optionally end at an offset ( resume download functionality )
                        )
{
 int res = 0;

 //This is the "serve straight from disk , not from the in-memory cache" fallback ( doNOTCacheRule , a file too
 //big to cache , etc ) - cache_GetResource()'s own cache-population path already runs this same containment
 //check ( see file_caching.c ) before a file is first cached , but a file that lands here specifically SKIPS
 //that step , so it needs its own check right here. verified_filename has already been through
 //FilenameStripperOk()'s string-level validation , but no character-level check can catch a symlink placed
 //inside webserver_root pointing somewhere else - that depends on the actual filesystem structure.
 if (!PathResolvesWithinDirectory(instance->webserver_root,verified_filename))
   {
    AmmServer_Error("TransmitFileToSocket : \"%s\" resolves outside the web root - refusing to serve it",verified_filename);
    return 0;
   }

 FILE * pFile = fopen (verified_filename, "rb" );

 //If we can't open the file we fail to transmit the file
 if (pFile==0)
   {
    fprintf(stderr,"Could not open file %s , files open %lu \n",verified_filename,instance->statistics.filesCurrentlyOpen);
    return 0;
   } else
   {
    //Count the open file
    ++instance->statistics.filesCurrentlyOpen;

     //Try to obtain file size if not fail to transmit the fail
     if ( fseek (pFile , 0 , SEEK_END)!=0 )
      {
        errorID(ASV_ERROR_COULD_NOT_FIND_FILESIZE);
        fclose(pFile);
        if (instance->statistics.filesCurrentlyOpen>0) { --instance->statistics.filesCurrentlyOpen; }  //Count the closed file
        return 0;
      }
      unsigned long lSize = ftell (pFile);
      //-------------------------------------------------------

    if ( (end_at_byte!=0) && (lSize<end_at_byte) )
      {
        fprintf(stderr,"Incorrect range request , the file changed? ( from %u to %u file 0 to %u ..! ) ,forcing known size\n",(unsigned int) start_at_byte,(unsigned int) end_at_byte,(unsigned int) lSize);
        end_at_byte=lSize;
      }

    fprintf(stderr,"Sending => Size %0.2f KB / Open files %lu / Filename %s \n",(double) lSize/1024,instance->statistics.filesCurrentlyOpen,verified_filename);

     if (
          TransmitFileHeaderToSocket(
                                     instance,
                                     transaction,
                                     start_at_byte,     // Optionally start with an offset ( resume download functionality )
                                     end_at_byte,       // Optionally end at an offset ( resume download functionality )
                                     lSize
                                     )
         )
     {
        rewind (pFile);
        if (start_at_byte!=0) { fseek (pFile , start_at_byte , SEEK_SET); }
        if (end_at_byte==0)   { end_at_byte=lSize-start_at_byte; }

        res = TransmitFileToSocketInternal(instance,pFile,transaction,end_at_byte);
     }


    fclose (pFile);
    if (instance->statistics.filesCurrentlyOpen>0) { --instance->statistics.filesCurrentlyOpen; }  //Count the closed file
  }
  return res;
}

int TransmitFileBodyToSocket(
                              struct AmmServer_Instance * instance,
                              struct HTTPTransaction * transaction,
                              const char * verified_filename,
                              unsigned long offset,
                              unsigned long byteCount
                            )
{
  if (byteCount==0) { return 1; }

  //Same containment check TransmitFileToSocket() performs : strings can be validated cheaply ,
  //but only realpath() catches a symlink inside the web root pointing somewhere else.
  if (!PathResolvesWithinDirectory(instance->webserver_root,verified_filename))
   {
     AmmServer_Error("TransmitFileBodyToSocket : \"%s\" resolves outside the web root - refusing to serve it",verified_filename);
     return 0;
   }

  FILE * pFile = fopen(verified_filename,"rb");
  if (pFile==0)
   {
     fprintf(stderr,"Could not open file %s for a range transmission\n",verified_filename);
     return 0;
   }

  ++instance->statistics.filesCurrentlyOpen;

  int res=0;
  if (fseek(pFile,(long) offset,SEEK_SET)==0)
   {
     res=TransmitFileToSocketInternal(instance,pFile,transaction,byteCount);
   } else
   {
     fprintf(stderr,"Could not seek to byte %lu of %s\n",offset,verified_filename);
   }

  fclose(pFile);
  if (instance->statistics.filesCurrentlyOpen>0) { --instance->statistics.filesCurrentlyOpen; }
  return res;
}

unsigned long SendErrorFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    unsigned int errorCode
  )
{
  AmmServer_Warning("Serving back new embedded error code ( for error %u ) ",errorCode);

  char * what2RespondWith=0;
  switch (errorCode)
  {
      case 400 : what2RespondWith=error400; break;
      case 401 : what2RespondWith=error401; break;
      case 403 : what2RespondWith=error403; break;
      case 404 : what2RespondWith=error404; break;
      case 408 : what2RespondWith=error408; break;
      case 412 : what2RespondWith=error412; break;
      case 413 : what2RespondWith=error413; break;
      case 416 : what2RespondWith=error416; break;
      case 417 : what2RespondWith=error417; break;
      case 500 : what2RespondWith=error500; break;
  };

  unsigned long bodyLength=0;
  if (what2RespondWith!=0) { bodyLength=(unsigned long) strlen(what2RespondWith); }

  if (!SendErrorResponse(instance,transaction,errorCode,what2RespondWith,bodyLength,0))
   { fprintf(stderr,"Error sending Error Response\n"); return 0; }

  return bodyLength;
}

unsigned long SendEmbeddedFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    char * verified_filename_pending_copy
  )
{
  unsigned long dataTransmitted=0;
  AmmServer_Warning("Serving back new embedded file ( %s ) ",verified_filename_pending_copy);

  SendSuccessCodeHeader(
                       instance,
                       transaction,
                       200,
                       verified_filename_pending_copy
                     );

  unsigned char * what2RespondWith=0;
  unsigned int responseSize=0;
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "dir.gif")==0)  { what2RespondWith=original_dir_gif;  responseSize=sizeof(original_dir_gif);  } else
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "doc.gif")==0)  { what2RespondWith=original_doc_gif;  responseSize=sizeof(original_doc_gif);  } else
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "exe.gif")==0)  { what2RespondWith=original_exe_gif;  responseSize=sizeof(original_exe_gif);  } else
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "img.gif")==0)  { what2RespondWith=original_img_gif;  responseSize=sizeof(original_img_gif);  } else
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "vid.gif")==0)  { what2RespondWith=original_vid_gif;  responseSize=sizeof(original_vid_gif);  } else
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "mus.gif")==0)  { what2RespondWith=original_mus_gif;  responseSize=sizeof(original_mus_gif);  } else
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "back.gif")==0) { what2RespondWith=original_back_gif; responseSize=sizeof(original_back_gif); } else
  if (strcmp(verified_filename_pending_copy,"public_html/" TEMPLATE_INTERNAL_URI "up.gif")==0)   { what2RespondWith=original_up_gif;   responseSize=sizeof(original_up_gif);   }

  if (what2RespondWith!=0)
  {
   AmmServer_Warning("The size to transmit is ( %u ) ",responseSize);
   char contentLength[128];
   snprintf(contentLength,128,"Content-Length: %u\n\n",responseSize);
   //send(transaction->clientSock,contentLength,strlen(contentLength),MSG_WAITALL|MSG_NOSIGNAL);
   ASRV_Send(instance, transaction , contentLength, strlen(contentLength), MSG_WAITALL|MSG_NOSIGNAL );


   //Send E-Tag as soon as we've got it
   int opres=ASRV_Send( instance, transaction, what2RespondWith, responseSize, MSG_WAITALL|MSG_NOSIGNAL );
   if (opres<=0) { fprintf(stderr,"Error sending Error Response\n"); return 0; } else
                 { dataTransmitted+=opres; }
  } else
  {
     //TODO : generic error here..
  }

   return dataTransmitted;
  return 0;
}

/*Resolve a parsed Range: header against the representation size.
  Returns 1=partial content , 0=no range/ignored , -1=unsatisfiable ( 416 ).*/
static int ResolveByteRange(
                             const struct HTTPHeader * request,
                             unsigned long representationSize,
                             unsigned long * rangeStart,
                             unsigned long * rangeEnd,
                             unsigned long * rangeLength
                           )
{
  *rangeStart=0;
  *rangeEnd=0;
  *rangeLength=representationSize;

  if ( (!request->rangeRequested) || (request->rangeKind==HTTP_RANGE_NONE) ) { return 0; }
  if (representationSize==0) { return -1; } //Any range on an empty representation is unsatisfiable

  switch (request->rangeKind)
  {
    case HTTP_RANGE_SUFFIX :
      if (request->range_start==0) { return -1; }
      if (request->range_start>=representationSize) { *rangeStart=0; }
                                                else { *rangeStart=representationSize-request->range_start; }
      *rangeEnd=representationSize-1;
    break;

    case HTTP_RANGE_OPEN_ENDED :
      *rangeStart=request->range_start;
      if (*rangeStart>=representationSize) { return -1; }
      *rangeEnd=representationSize-1;
    break;

    case HTTP_RANGE_CLOSED :
      *rangeStart=request->range_start;
      if (*rangeStart>=representationSize) { return -1; }
      *rangeEnd=request->range_end;
      if (*rangeEnd>=representationSize) { *rangeEnd=representationSize-1; }
    break;

    default : return 0; break;
  }

  *rangeLength=(*rangeEnd-*rangeStart)+1;
  return 1;
}

static unsigned long SendRangeNotSatisfiable(
                                              struct AmmServer_Instance * instance,
                                              struct HTTPTransaction * transaction,
                                              unsigned long representationSize
                                            )
{
  char extraHeaders[128]={0};
  snprintf(extraHeaders,sizeof(extraHeaders),"Content-Range: bytes */%lu\n",representationSize);
  return SendErrorResponse(
                            instance,
                            transaction,
                            416,
                            error416,
                            (unsigned long) strlen(error416),
                            extraHeaders
                          );
}

unsigned long SendFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    char * verified_filename_pending_copy, // The filename to be served on the socket above
    unsigned int force_error_code
  )
{
 if (force_error_code!=0)
 {
  //No point going any further , we are actually talking about a SendErroFile call..!
  return SendErrorFile(instance,transaction,force_error_code);
 }

 unsigned int resourceCacheID=transaction->resourceCacheID;
 unsigned char keepalive = transaction->incomingHeader.keepalive;

  struct HTTPHeader * request = &transaction->incomingHeader;

  char verified_filename[MAX_FILE_PATH+1]={0};
  char reply_header[MAX_HTTP_REQUEST_HEADER_REPLY+1]={0};

  if (verified_filename_pending_copy != 0)
  {
     strncpy(verified_filename,verified_filename_pending_copy,MAX_FILE_PATH);
  }

  //FilenameStripperOk() is a string-level check ( always safe/cheap , runs on every request ) ; a symlink
  //placed inside webserver_root pointing somewhere else passes it with no trouble at all, since nothing about
  //the request string itself is wrong - that's exactly why PathResolvesWithinDirectory() also gets checked
  //here, before any header goes out , so a rejected request gets a proper error instead of a 200 header
  //already having gone out with no body to follow.
  if ( (!FilenameStripperOk(verified_filename)) || (!PathResolvesWithinDirectory(instance->webserver_root,verified_filename)) )
  {
     SendErrorFile(instance,transaction,400);
     return 1;
  }

  struct cache_item * cache = (struct cache_item *) instance->cache;

  int opres=0;
  unsigned int index=0;
  unsigned long cached_lSize=0;
  unsigned char cached_buffer_is_compressed = request->supports_compression;
  unsigned char allowOtherOrigins=0;
  unsigned char free_cached_buffer_after_use=0;
  unsigned char serveAsRegularFile=0;
  //A dynamic-content callback can ask for a status code other than 200 by setting rqst->headerResponse ; this
  //is where whatever it asked for comes back ( 0 = it didn't ask , serve the ordinary 200 ).
  unsigned int dynamicResponseCode=0;
  char * cached_buffer = cache_GetResource(instance,request,resourceCacheID,verified_filename,MAX_FILE_PATH,&index,&cached_lSize,0,&cached_buffer_is_compressed,&free_cached_buffer_after_use,&serveAsRegularFile,&allowOtherOrigins,transaction->pendingResponseHeaders,sizeof(transaction->pendingResponseHeaders),&dynamicResponseCode);

  //serveAsRegularFile leaves compressionSupported untouched ; a plain file is never deflated
  if (cached_buffer==0) { cached_buffer_is_compressed=0; }

  unsigned char isDynamicMemory = ( (cached_buffer!=0) && (cache[index].dynamicRequestCallbackFunction!=0) && (!serveAsRegularFile) );

  unsigned long representationSize=0;
  time_t representationModificationTime=0;
  unsigned char haveModificationTime=0;

  if (isDynamicMemory)
  {
    //Callback-generated memory : its size is whatever the callback produced , and it has no
    //filesystem modification time. Range/conditional handling is intentionally skipped for this.
    representationSize=cached_lSize;
  } else
  if ( (cached_buffer!=0) && (!serveAsRegularFile) )
  {
    //Served from the static cache : size and mtime must describe the cached bytes , not whatever
    //the file happens to look like on disk now (the cache may be stale by design).
    representationSize=cached_lSize;
    representationModificationTime=cache[index].modification_time;
    haveModificationTime=(representationModificationTime!=0);
  } else
  {
    //Streamed from disk - this includes a dynamic callback that returned a filename to stream.
    struct stat fileToStream;
    if ( (stat(verified_filename,&fileToStream)!=0) || (!S_ISREG(fileToStream.st_mode)) ||
         (!PathResolvesWithinDirectory(instance->webserver_root,verified_filename)) )
    {
      AmmServer_Warning("%s cannot be streamed from disk and is not cached , answering 404",verified_filename);
      freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);
      return SendErrorFile(instance,transaction,404);
    }
    representationSize=(unsigned long) fileToStream.st_size;
    representationModificationTime=fileToStream.st_mtime;
    haveModificationTime=1;
  }

  //Local ETag , only cached resources have a hash
  char LocalETag[MAX_ETAG_SIZE]={0};
  if (cached_buffer!=0)
  {
    unsigned int cache_etag = cache_GetHashOfResource(instance,index);
    if (cache_etag!=0)
     { snprintf(LocalETag,MAX_ETAG_SIZE,"%u%u%lu%lu",instance->cacheVersionETag,cache_etag,request->range_start,request->range_end); }
  }

  //Preconditions for static GET/HEAD
  unsigned char not_modified=0;
  if (isDynamicMemory)
  {
    //Preserve the historical dynamic-page ETag/cooldown behavior : a callback that is still
    //inside its callback_every_x_msec cooldown can answer 304 when its ETag matches.
    if ( (request->eTag!=0) && (LocalETag[0]!=0) && (!cache[index].doNOTCacheRule) &&
         (cache[index].dynamicRequest!=0) && (cache[index].dynamicRequest->callback_cooldown) &&
         (strncmp(request->eTag,LocalETag,request->eTagLength)==0) )
     { not_modified=1; }
  } else
  if ( (request->requestType==GET) || (request->requestType==HEAD) )
  {
    if ( request->ifUnmodifiedSinceSet && haveModificationTime &&
         (representationModificationTime > request->ifUnmodifiedSince) )
    {
      freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);
      return SendErrorFile(instance,transaction,412);
    }

    if (request->eTag!=0)
    {
      //If-None-Match takes precedence : If-Modified-Since is ignored whenever it is present
      if ( (LocalETag[0]!=0) && (strncmp(request->eTag,LocalETag,request->eTagLength)==0) ) { not_modified=1; }
    } else
    if ( request->ifModifiedSinceSet && haveModificationTime &&
         (representationModificationTime <= request->ifModifiedSince) )
    {
      not_modified=1;
    }
  }

  if (not_modified)
  {
    if (!SendNotModifiedHeader(instance,transaction))
     { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }

    if (haveModificationTime)
     {
       if (GetDateStringFromTimeT(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"Last-Modified",representationModificationTime))
        { if (!SendPart(instance,transaction,reply_header,strlen(reply_header),1)) { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; } }
     }

    if (LocalETag[0]!=0)
     {
       char etagChunk[MAX_ETAG_SIZE+64]={0};
       snprintf(etagChunk,sizeof(etagChunk),"ETag: \"%s\" \n",LocalETag);
       if (!SendPart(instance,transaction,etagChunk,strlen(etagChunk),1)) { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }
     }

    if (keepalive) { SendPart(instance,transaction,"Connection: keep-alive\n",strlen("Connection: keep-alive\n"),0); }
              else { SendPart(instance,transaction,"Connection: close\n",strlen("Connection: close\n"),0); }
    SendPart(instance,transaction,"\n",strlen("\n"),0);

    freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);
    return 1;
  }

  //Range resolution ( static , identity representation , GET/HEAD only )
  unsigned long rangeStart=0,rangeEnd=0,rangeLength=representationSize;
  unsigned char partialContent=0;
  if ( (!isDynamicMemory) && (!cached_buffer_is_compressed) &&
       ( (request->requestType==GET) || (request->requestType==HEAD) ) &&
       (request->rangeRequested) && (request->rangeKind!=HTTP_RANGE_NONE) )
  {
    int rangeResult=ResolveByteRange(request,representationSize,&rangeStart,&rangeEnd,&rangeLength);
    if (rangeResult<0)
     {
       freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);
       return SendRangeNotSatisfiable(instance,transaction,representationSize);
     }
    partialContent=(rangeResult>0);
  }

  //Send the status line only after size , preconditions and range are resolved
  unsigned int responseCode=partialContent ? 206 : ( (dynamicResponseCode!=0) ? dynamicResponseCode : 200 );
  if (!SendSuccessCodeHeader(instance,transaction,responseCode,verified_filename))
  {
    fprintf(stderr,"Failed sending success code \n");
    freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);
    return 0;
  }

  if (allowOtherOrigins)
  {
    AmmServer_Warning("Allowing other origins ( cross scripting..? )");
    if (!SendPart(instance,transaction,"Access-Control-Allow-Origin: *\n",strlen("Access-Control-Allow-Origin: *\n"),1))
     { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }
  }

  if ( (!isDynamicMemory) && (haveModificationTime) )
  {
    if (GetDateStringFromTimeT(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"Last-Modified",representationModificationTime))
     {
       if (!SendPart(instance,transaction,reply_header,strlen(reply_header),1))
        { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }
     }
  }

  if (LocalETag[0]!=0)
  {
    snprintf(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"ETag: \"%s\"\n",LocalETag);
    if (!SendPart(instance,transaction,reply_header,strlen(reply_header),1))
     { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }
  }

  if (cached_buffer_is_compressed)
  {
    if (!SendPart(instance,transaction,"Content-Encoding: deflate\n",strlen("Content-Encoding: deflate\n"),1))
     { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }
  }

  //Content-length/body only follow when we're about to send a body ( not a HEAD response )
  int headRequest=(request->requestType==HEAD);
  int connection_header_more_follows = (!headRequest);
  if (keepalive) { if (!SendPart(instance,transaction,"Connection: keep-alive\n",strlen("Connection: keep-alive\n"),connection_header_more_follows) ) { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }  } else
                 { if (!SendPart(instance,transaction,"Connection: close\n",strlen("Connection: close\n"),connection_header_more_follows)) { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }  }

  if (partialContent)
  {
    snprintf(
              reply_header,
              MAX_HTTP_REQUEST_HEADER_REPLY,
              "Content-Range: bytes %lu-%lu/%lu\r\nContent-Length: %lu\r\n\r\n",
              rangeStart,
              rangeEnd,
              representationSize,
              rangeLength
            );
  } else
  {
    snprintf(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"Content-Length: %lu\r\n\r\n",representationSize);
  }

  if (!SendPart(instance,transaction,reply_header,strlen(reply_header),!headRequest))
   { freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use); return 0; }

  if (headRequest)
  {
    freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);
    return 1;
  }

  if (cached_buffer!=0)
  {
    unsigned long bodyOffset=(partialContent?rangeStart:0);
    unsigned long bodyLength=(partialContent?rangeLength:representationSize);

    if (cached_lSize==0) { errorID(ASV_ERROR_CACHE_HAS_NO_PAYLOAD); }

    if (bodyLength>0)
     {
       opres=ASRV_Send(
                        instance,
                        transaction,
                        cached_buffer+bodyOffset,
                        bodyLength,
                        MSG_WAITALL|MSG_NOSIGNAL
                      );  //Last part of the response , no MSG_MORE
       freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);

       if (opres<=0) { fprintf(stderr,"Error sending cached body\n"); return 0; }
     } else
     {
       freeMallocIfNeeded(cached_buffer,free_cached_buffer_after_use);
     }
    return 1;
  }

  //Stream the requested slice straight from disk
  unsigned long bodyOffset=(partialContent?rangeStart:0);
  unsigned long bodyLength=(partialContent?rangeLength:representationSize);
  if (!TransmitFileBodyToSocket(instance,transaction,verified_filename,bodyOffset,bodyLength))
   {
     fprintf(stderr,"Could not transmit file %s \n",verified_filename);
     return 0;
   }

  return 1;
}

unsigned long SendMemoryBlockAsFile
  (
    struct AmmServer_Instance * instance,
    char * filename,
    struct HTTPTransaction * transaction,
    //char * path, // The filename to be served on the socket above
    char * mem, // The memory block body to be sent
    unsigned long mem_block // The size of the memory block to be sent
  )
{
  if (! SendSuccessCodeHeader(instance,transaction,200,filename)) { fprintf(stderr,"Failed sending success code \n"); return 0; }


  char reply_header[MAX_HTTP_REQUEST_HEADER_REPLY+1]={0};
  snprintf(reply_header,MAX_HTTP_REQUEST_HEADER_REPLY,"Content-Length: %u\nConnection: close\r\n\r\n",(unsigned int) mem_block);
  //TODO : Location : path etc

  if (!SendPart(instance,transaction,reply_header,strlen(reply_header),1)) //the body always follows this
      { fprintf(stderr,"Failed sending content length @  SendMemoryBlockAsFile ..!\n");  }

  if (!SendPart(instance,transaction,mem,mem_block,0)) //the last part of this response
      { fprintf(stderr,"Failed sending content body @  SendMemoryBlockAsFile ..!\n");  }

 return 0;
}
