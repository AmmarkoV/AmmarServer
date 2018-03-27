
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include <poll.h>

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



#define MICROSECONDS_TO_WAIT_FOR_RESPONSE 1500 //1.5 msec round trip time should be enough?

#define BUFFERSIZE 4096

int AmmClient_SendFileInternal(
                       struct AmmClient_Instance * instance,
                       const char * URI ,
                       const char * formname,
                       const char * filename ,
                       const char * filecontent ,
                       unsigned int filecontentSize,
                       int keepAlive
                      )
{
  //NOT TESTED
  #warning "TODO: Randomize File Boundaries on POST requests"
  char boundary[33]={"----AmmClientBoundaryAbcdefghijk"};
  char buffer[BUFFERSIZE];
  snprintf(buffer,BUFFERSIZE,"POST %s HTTP/1.1\nContent-Type: multipart/form-data; boundary=%s\n%s\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\n \n",
           URI,
           boundary,
           boundary,
           formname,
           filename
          );


  int step1=AmmClient_SendInternal( instance, buffer , strlen(buffer) , keepAlive );
  int step2=AmmClient_SendInternal( instance, filecontent , filecontentSize , keepAlive );

  snprintf(buffer,BUFFERSIZE,"%s\n",boundary );

  int step3=AmmClient_SendInternal( instance, buffer , strlen(buffer) , keepAlive );


 return ( (step1)&&(step2)&&(step3));
}





int AmmClient_RecvFileInternalClean(
                       struct AmmClient_Instance * instance,
                       const char * URI ,
                       char * filecontent ,
                       unsigned int * filecontentSize,
                       int keepAlive
                      )
{
 snprintf(filecontent,*filecontentSize,"GET /%s HTTP/1.1\nConnection: keep-alive\n\n",URI);

 unsigned int bytesReceived=0;

 if (AmmClient_Send(instance,filecontent,strlen(filecontent),keepAlive))
 {
     memset(filecontent,0,strlen(filecontent));
     unsigned int doneReceiving=0;

     usleep(MICROSECONDS_TO_WAIT_FOR_RESPONSE);

     while (!doneReceiving)
     {
       /* Upon successful completion, recv() shall return the length of the message in bytes.
        If no messages are available to be received and the peer has performed an orderly shutdown,
        recv() shall return 0.*/
       int result = recv(instance->clientSocket, filecontent+bytesReceived, *filecontentSize-bytesReceived, 0);
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
                           bytesReceived+=result;
                           //fprintf(stderr,GREEN" Received %u more bytes ( total %u ) \n" NORMAL , result , bytesReceived);
                         }

       if (bytesReceived>=*filecontentSize) { doneReceiving=1;}
       if (isFileDownloadComplete(filecontent,bytesReceived))
                         {  *filecontentSize = bytesReceived; return 1; }

     }
 }

  *filecontentSize = bytesReceived;

  AmmClient_CloseDeadConnectionIfNeeded(instance);
  AmmClient_CheckConnectionInternal(instance);

 return 0;
}


int isFileDownloadComplete(const char * content,unsigned int contentSize)
{
 //fprintf(stderr," Content (%u bytes) : %s \n" ,contentSize , content);
  char * contentLengthStr=strstr(content,"Content-length:");
  if (contentLengthStr!=0)
  {
      contentLengthStr+=16;
      unsigned int contentLength = atoi(contentLengthStr);
      //fprintf(stderr,YELLOW " Content Length = %u  \n" NORMAL , contentLength );
      if (contentLength!=0)
      {
        unsigned int headerSize = contentLengthStr - content;

        if (headerSize+contentLength <= contentSize)
              {
                //fprintf(stderr,GREEN " File fully downloaded ( %u header + %u content = %u we have )\n" NORMAL , headerSize , contentLength , contentSize);
                return 1;
              }
      }
  }
  //fprintf(stderr,RED " File not fully downloaded\n" NORMAL );
 return 0;
}






int AmmClient_RecvFileInternal(
                       struct AmmClient_Instance * instance,
                       const char * URI ,
                       char * filecontent ,
                       unsigned int * filecontentSize,
                       int keepAlive
                      )
{
/*
 //This is clean and works  @ 25Hz
 return AmmClient_RecvFileInternalClean(
                                        instance,
                                        URI ,
                                        filecontent ,
                                        filecontentSize,
                                        keepAlive
                                       );
*/



 snprintf(filecontent,*filecontentSize,"GET /%s HTTP/1.1\nConnection: keep-alive\n\n",URI);

 unsigned int bytesReceived=0;

 if (AmmClient_Send(instance,filecontent,strlen(filecontent),keepAlive))
 {
     memset(filecontent,0,strlen(filecontent));
     unsigned int doneReceiving=0;
     unsigned int recvCalled=0;
     unsigned int connectionHalted=0;
     int result =0;

     struct pollfd fd;
     int ret;

     usleep(MICROSECONDS_TO_WAIT_FOR_RESPONSE);


     while (!doneReceiving)
     {
      recvCalled=0;

      //Do ultra fast polling to check if connection has data..
      //---------------------------------------------------------------------
      fd.fd = instance->clientSocket; // your socket handler
      fd.events = POLLIN;
      ret = poll(&fd, 1, 1); // 1ms for timeout
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
                       result=recv(instance->clientSocket, filecontent+bytesReceived, *filecontentSize-bytesReceived, 0);
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
                           bytesReceived+=result;
                           //fprintf(stderr,GREEN" Received %u more bytes ( total %u ) \n" NORMAL , result , bytesReceived);
                          }

        if (bytesReceived>=*filecontentSize) { doneReceiving=1;}

        if (isFileDownloadComplete(filecontent,bytesReceived))
          {
            *filecontentSize = bytesReceived;
            return 1;
          }

       }
     }
 }

  *filecontentSize = bytesReceived;

  //AmmClient_CloseDeadConnectionIfNeeded(instance);
  //AmmClient_CheckConnectionInternal(instance);

 return 0;
}
