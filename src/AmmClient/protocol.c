
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

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
        #warning "TODO : isFileDownloadComplete check that contentLength has been reached by content Size"
        //fprintf(stderr,GREEN " File ALMOST fully downloaded TODO ALSO CHECK HEADER SIZE\n" NORMAL );
        return 1;
      }
  }
// fprintf(stderr,RED " File not fully downloaded\n" NORMAL );
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
 snprintf(filecontent,*filecontentSize,"GET /%s HTTP/1.1\nConnection: keep-alive\n\n",URI);
 if (AmmClient_Send(instance,filecontent,strlen(filecontent),keepAlive))
 {
     memset(filecontent,0,strlen(filecontent));
     unsigned int doneReceiving=0;
     unsigned int bytesReceived=0;
     unsigned int connectionHalted=0;

     //fprintf(stderr,RED " Waiting to receive response : \n" NORMAL);
     while (!doneReceiving)
     {
       int result = recv(instance->clientSocket, filecontent+bytesReceived, *filecontentSize-bytesReceived, 0);
       if (result == 0 ) {
                            fprintf(stderr,".");
                            ++connectionHalted;
                            //usleep(100);
                            if (connectionHalted>5 /*Maximum connection hiccup*/) { doneReceiving=1; }
                         } else
       if (result < 0 ) {
                           fprintf(stderr,RED "Failed to Recv error : %u\n" NORMAL,errno);
                           AmmClient_CloseDeadConnectionIfNeeded(instance);
                           return 0;
                         } else
                         {
                           bytesReceived+=result;
                           //fprintf(stderr,GREEN" Received %u more bytes ( total %u ) \n" NORMAL , result , bytesReceived);
                         }

       if (isFileDownloadComplete(filecontent,*filecontentSize)) { return 1; }
       if (bytesReceived>=*filecontentSize) { doneReceiving=1;}
     }
 }

  AmmClient_CloseDeadConnectionIfNeeded(instance);
  AmmClient_CheckConnectionInternal(instance);

 return 0;
}
