#include <stdio.h>
#include <unistd.h>

#include "AmmClient.h"



int main(int argc, char *argv[])
{
 fprintf(stderr,"AmmClient Tester started \n");
 struct AmmClient_Instance * inst = AmmClient_Initialize("192.168.1.48",8080);
 fprintf(stderr,"Initialized..\n");

 if (inst)
 {
  char buf[1024]={0};
  unsigned int recvdSize=0;

  unsigned long startTime,endTime;

  unsigned int i=0;
   while (1)
  {
   startTime = AmmClient_GetTickCountMicroseconds();

   snprintf(buf,1024,"GET /index.html?test=%u HTTP/1.1\nConnection: keep-alive\n\n",i);

   fprintf(stderr,"Send #%u..\n",i);
   if (AmmClient_Send(inst,buf,strlen(buf),1))
   {
    usleep(100);
    fprintf(stderr,"Recv #%u..\n",i);
    recvdSize=1024;

    if (!AmmClient_Recv(inst,buf,&recvdSize) )
      {
       fprintf(stderr,"Failed to recv.. \n");
      }


   endTime = AmmClient_GetTickCountMicroseconds();

   fprintf(stderr,"Took %lu microseconds \n",endTime-startTime);
   } else
   {
     fprintf(stderr,"Failed to Send.. \n");
   }
   usleep(1000);

   ++i;

  }

  AmmClient_Close(inst);
 }

  return 0;
}
