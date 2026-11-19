
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <strings.h> /* strncasecmp */

#include <poll.h>

#include "AmmClient.h"
#include "protocol.h"
#include "network.h"



#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */



#define MICROSECONDS_TO_WAIT_FOR_RESPONSE 2000 //2 msec round trip time should be enough , ultimately this only matters in reallyFastImplementation since the regular
//will wait until the recv is complete

#define BUFFERSIZE 4096

int AmmClientTransmitBoundary(struct AmmClient_Instance * instance,const char * boundary,int keepAlive,int first,int after,int last)
{
  char transmission[256]={"------------------------8a7a8e26954c7dff"};
  int transmissionSize = 0;

  if (first)
  {
    transmissionSize = snprintf(transmission,256,"--%s\r\n",boundary);
  } else
  if (after)
  {
    transmissionSize = snprintf(transmission,256,"\r\n--%s\r\n",boundary);
  } else
  if (last)
  {
    transmissionSize = snprintf(transmission,256,"\r\n--%s--\r\n",boundary);
  } else
  {
    transmissionSize = snprintf(transmission,256,"\r\n--%s\r\n",boundary);
  }

  //fprintf(stderr,"%s",transmission);

return AmmClient_SendInternal(instance,transmission,transmissionSize,keepAlive);

}


//https://www.w3schools.com/php/php_file_upload.asp
//nc -l 0.0.0.0 8080
//curl -F "submit=1" -F "fileToUpload=@/home/dji/catkin_ws/src/camera_broadcast/src/image.jpg" ammar.gr/stream/upload.php
//curl -F "fileToUpload=@/home/ammar/Documents/Programming/AmmarServer/src/AmmClient/test.jpg" -F "submit=1"  ammar.gr/stream/upload.php
int AmmClient_SendFileInternal(
                       struct AmmClient_Instance * instance,
                       const char * URI ,
                       const char * formname,
                       const char * filename ,
                       const char * contentType,
                       const char * filecontent ,
                       unsigned int filecontentSize,
                       int keepAlive
                      )
{
  //Ok first of all we want to generate a random boundary..
  char boundary[128]={0};
  unsigned int rpA=rand()%9999;
  unsigned int rpB=rand()%9999;
  unsigned int rpC=rand()%9999;
  unsigned int rpD=rand()%9999;
  unsigned int boundarySize = snprintf(boundary,128,"------------------------%04u%04u%04u%04u",rpA,rpB,rpC,rpD);
  //--------------------------------------------------------------

  //Our transmission consists of 4 parts
  // 1st HEADER
  // 2nd FILE PREAMBLE
  // 3d  BINARY FILA
  // 4th POST FILE STUFF

  int success = 0;
  int steps   = 0;

  //We prepare them before sending the file to be able to know the content length of the file
  unsigned int bufferBeforeSize=0;
  char bufferBeforeFile[BUFFERSIZE+1]={0};
  bufferBeforeSize = snprintf(bufferBeforeFile,BUFFERSIZE,"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",formname,filename,contentType);

  unsigned int bufferAfterSize=0;
  char bufferAfterFile[BUFFERSIZE+1]={0};
  bufferAfterSize = snprintf(bufferAfterFile,BUFFERSIZE,"Content-Disposition: form-data; name=\"submit\"\r\n\r\n1");


  //snprintf returns what it would have written , sending that many bytes from a truncated buffer read past it
  if ( (bufferBeforeSize>=BUFFERSIZE) || (bufferAfterSize>=BUFFERSIZE) ) { fprintf(stderr,"AmmClient_SendFile : form or file name too long\n"); return 0; }

  unsigned int contentLength = bufferBeforeSize + boundarySize + 4 + filecontentSize + boundarySize + 6 + bufferAfterSize + boundarySize + 8;


  char header[BUFFERSIZE+1]={0};
  //The server only keeps the connection open when the request asks for it
  //(AnalyzeHTTPLineRequest sets keepalive from the Connection: header), so when
  //the caller wants keepAlive we have to say so or the reply is followed by a
  //close and our next send() gets an ECONNRESET.
  int headerSize = snprintf(header,BUFFERSIZE,"POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: AmmClient/%s\r\nAccept: */*\r\nContent-Length: %u\r\nContent-Type: multipart/form-data; boundary=%s\r\nConnection: %s\r\n\r\n",URI,instance->ip,AmmClientVersion,contentLength,boundary,(keepAlive)?"keep-alive":"close");
  if ( (headerSize<0) || (headerSize>=BUFFERSIZE) ) { fprintf(stderr,"AmmClient_SendFile : URI too long\n"); return 0; }

  //Send everything..
  ++steps; success+=AmmClient_SendInternal(instance,header,headerSize,keepAlive);
  ++steps; success+=AmmClientTransmitBoundary(instance,boundary,keepAlive,1,0,0);
  ++steps; success+=AmmClient_SendInternal(instance,bufferBeforeFile,bufferBeforeSize,keepAlive);
  ++steps; success+=AmmClient_SendInternal(instance,filecontent,filecontentSize,keepAlive);
  ++steps; success+=AmmClientTransmitBoundary(instance,boundary,keepAlive,0,1,0);
  ++steps; success+=AmmClient_SendInternal(instance,bufferAfterFile,bufferAfterSize,keepAlive);
  ++steps; success+=AmmClientTransmitBoundary(instance,boundary,keepAlive,0,0,1);

  //Maybe response should be parsed here..
 return ( success==steps );
}





//content must be terminated at contentSize ( the receive loops below keep it so ). A response is complete once its
//headers have ended and Content-Length body bytes followed them. This used to measure the header only up to the
//Content-Length value ( so any header after it made a download stop early , truncated ) , matched the header name
//case-sensitively , and never finished a Content-Length: 0 response.
int isFileDownloadComplete(const char * content,unsigned int contentSize)
{
  const char * endOfHeaders = strstr(content,"\r\n\r\n");
  if (endOfHeaders==0) { return 0; }
  unsigned int headerSize = (unsigned int) (endOfHeaders - content) + 4;

  const char * line = strstr(content,"\r\n"); //Skip the status line
  while ( (line!=0) && (line<endOfHeaders) )
  {
    line+=2;
    if (strncasecmp(line,"Content-Length:",15)==0)
    {
      char * end=0;
      unsigned long contentLength = strtoul(line+15,&end,10);
      if (end==line+15) { return 0; } //No number
      return ( (unsigned long) headerSize + contentLength <= contentSize );
    }
    line = strstr(line,"\r\n");
  }
  //No Content-Length : the response ends when the server closes the connection
 return 0;
}



//nc -l 0.0.0.0 8080
//curl http://127.0.0.1:8080/stream/uploads/image.jpg -o downloaded.jpg
int AmmClient_RecvFileInternalClean(
                                    struct AmmClient_Instance * instance,
                                    const char * URI ,
                                    char * filecontent ,
                                    unsigned int * filecontentSize,
                                    int keepAlive
                                   )
{
     unsigned int bytesReceived=0;
     filecontent[0]=0; //Kept terminated at bytesReceived from here on , isFileDownloadComplete() relies on it
     unsigned int doneReceiving=0;

     usleep(MICROSECONDS_TO_WAIT_FOR_RESPONSE);

     while (!doneReceiving)
     {
       /* Upon successful completion, recv() shall return the length of the message in bytes.
        If no messages are available to be received and the peer has performed an orderly shutdown,
        recv() shall return 0.*/
       int result = recv(instance->clientSocket, filecontent+bytesReceived, *filecontentSize-1-bytesReceived, 0);
       if (result == 0 ) {
                            fprintf(stderr,".");
                            doneReceiving=1;
                         } else
       if (result < 0 )  {
                           fprintf(stderr,RED "Failed to Recv error : %u\n" NORMAL,errno);
                           AmmClient_CloseDeadConnectionIfNeeded(instance);
                           return 0;
                         } else
                         {
                           bytesReceived+=result; filecontent[bytesReceived]=0;
                           //fprintf(stderr,GREEN" Received %u more bytes ( total %u ) \n" NORMAL , result , bytesReceived);
                         }

       if (bytesReceived+1>=*filecontentSize) { doneReceiving=1;} //Full , the last byte is the terminator
       if (isFileDownloadComplete(filecontent,bytesReceived)) {  *filecontentSize = bytesReceived; return 1; }

     }

  *filecontentSize = bytesReceived;

  AmmClient_CloseDeadConnectionIfNeeded(instance);
  AmmClient_CheckConnectionInternal(instance);

 return 0;
}




int AmmClient_RecvFileInternalFast(
                       struct AmmClient_Instance * instance,
                       const char * URI ,
                       char * filecontent ,
                       unsigned int * filecontentSize,
                       int keepAlive
                      )
{
     unsigned int bytesReceived=0;
     filecontent[0]=0; //Kept terminated at bytesReceived from here on , isFileDownloadComplete() relies on it
     unsigned int doneReceiving=0;
     unsigned int connectionHalted=0;
     int result =0;

     struct pollfd fd;

     usleep(MICROSECONDS_TO_WAIT_FOR_RESPONSE);


     while (!doneReceiving)
     {
      int recvCalled=0;

      //Do ultra fast polling to check if connection has data..
      //---------------------------------------------------------------------
      fd.fd = instance->clientSocket; // your socket handler
      fd.events = POLLIN;
      int ret = poll(&fd, 1, 1); // 1ms for timeout
      switch (ret) {
                     case -1:
                      doneReceiving=1;
                     break;
                     case 0:
                        fprintf(stderr,".");
                        ++connectionHalted;

                        result=0;
                        doneReceiving=1;
                     break;
                     default:
                       result=recv(instance->clientSocket, filecontent+bytesReceived, *filecontentSize-1-bytesReceived, 0);
                       recvCalled=1;
                     break;
                   }
      //---------------------------------------------------------------------


       if (recvCalled)
       {
        if (result == 0 ) {
                            fprintf(stderr,"!");
                            ++connectionHalted;
                            doneReceiving=1;
                          } else
        if (result < 0 )  {
                           fprintf(stderr,RED "\nFailed to Recv error : %u\n" NORMAL,errno);
                           AmmClient_CloseDeadConnectionIfNeeded(instance);
                           return 0;
                          } else
                          {
                           bytesReceived+=result; filecontent[bytesReceived]=0;
                           //fprintf(stderr,GREEN" Received %u more bytes ( total %u ) \n" NORMAL , result , bytesReceived);
                          }

        if (bytesReceived+1>=*filecontentSize) { doneReceiving=1;} //Full , the last byte is the terminator

        if (isFileDownloadComplete(filecontent,bytesReceived))
          {
            *filecontentSize = bytesReceived;
            return 1;
          }

       }
     }

  *filecontentSize = bytesReceived;

  //AmmClient_CloseDeadConnectionIfNeeded(instance);
  //AmmClient_CheckConnectionInternal(instance);

 return 0;
}



int AmmClient_RecvFileInternal(
                       struct AmmClient_Instance * instance,
                       const char * URI ,
                       char * filecontent ,
                       unsigned int * filecontentSize,
                       int keepAlive ,
                       int reallyFastImplementation
                      )
{
 if ( (instance==0) || (URI==0) || (filecontent==0) || (filecontentSize==0) ) { return 0; }
 //HTTP/1.1 keeps connections open unless told otherwise , so a request that does not want keep-alive has to say
 //Connection: close ( it used to say nothing , and also ended its headers with one blank line too many )
 int requestSize = snprintf(filecontent,*filecontentSize,"GET %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nConnection: %s\r\n\r\n",
                            URI,instance->ip,(keepAlive) ? "keep-alive" : "close");
 if ( (requestSize<0) || ((unsigned int) requestSize+2 > *filecontentSize) )
  {
      fprintf(stderr,"AmmClient_RecvFileInternal not enough memory for performing a request..");
      return 0;
  }

 if (AmmClient_Send(instance,filecontent,(unsigned int) requestSize,keepAlive))
 {
   if (reallyFastImplementation)
    {
     //If we are here it means we are in a terrible hurry
     //The "fast" implementation uses poll to check sockets
     //and will not block, providing a much faster implementation..
     return AmmClient_RecvFileInternalFast(
                                           instance,
                                           URI ,
                                           filecontent ,
                                           filecontentSize,
                                           keepAlive
                                          );

    }
 else
    {
     //This is the clean and simple version for receiving files
     //it uses regular recv and works @ 25Hz due to system blocking
     //at recv for a predetermined amount of time.
     return AmmClient_RecvFileInternalClean(
                                            instance,
                                            URI ,
                                            filecontent ,
                                            filecontentSize,
                                            keepAlive
                                          );
    }
 }
 return 0;
}
