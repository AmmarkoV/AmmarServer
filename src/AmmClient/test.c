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
  for (i=0; i<10; i++)
  {
   startTime = AmmClient_GetTickCountMicroseconds();

   snprintf(buf,1024,"GET /index.html?test=%u HTTP/1.1\nConnection: keep-alive\n\n",i);

   fprintf(stderr,"Send %u..\n",i);
   AmmClient_Send(inst,buf,1024,1);
   usleep(100);
   fprintf(stderr,"Recv %u..\n",i);
   recvdSize=1024;
   AmmClient_Recv(inst,buf,&recvdSize);


   endTime = AmmClient_GetTickCountMicroseconds();

   fprintf(stderr,"Took %lu microseconds \n",endTime-startTime);
   usleep(1000);



  }

  AmmClient_Close(inst);
 }

  return 0;
}
