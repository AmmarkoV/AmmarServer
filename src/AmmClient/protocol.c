
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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










int AmmClient_RecvFileInternal(
                       struct AmmClient_Instance * instance,
                       const char * URI ,
                       char * filecontent ,
                       unsigned int * filecontentSize,
                       int keepAlive
                      )
{
 fprintf(stderr,RED "TODO: AmmClient_RecvFileInternal is idiotically implemented.. \n" NORMAL);
 char buffer[BUFFERSIZE]={0};
 snprintf(buffer,BUFFERSIZE,"GET /%s HTTP/1.1\nConnection: keep-alive\n\n",URI);
 if (AmmClient_Send(instance,buffer,strlen(buffer),keepAlive))
 {
     usleep(100000);
     if ( AmmClient_Recv(instance,filecontent,filecontentSize) )
     {
        //TODO: proper implementation here that will read the HTTP header..
       return 1;
     }
 }

 return 0;
}
