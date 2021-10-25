#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "AmmClient.h"



int spam()
{
     fprintf(stderr,"AmmClient Tester started \n");
 struct AmmClient_Instance * inst = AmmClient_Initialize("192.168.1.48",8080,10/*sec*/);
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

 return 1;
}

int stream()
{
 fprintf(stderr,"AmmClient Tester started \n");
 //struct AmmClient_Instance * inst = AmmClient_Initialize("127.0.0.1",8080,10/*sec*/);
 struct AmmClient_Instance * inst = AmmClient_Initialize("139.91.185.16",80,10/*sec*/);
 fprintf(stderr,"Initialized..\n");

 if (inst)
 {
  char buf[1024]={0};
  unsigned int recvdSize=0;

  unsigned long startTime,endTime;

  unsigned int jpegSize=0;
  //char * jpegImage = AmmClient_ReadFileToMemory("/home/ammar/tmp.txt",&jpegSize); //"test.jpg"
  char * jpegImage = AmmClient_ReadFileToMemory("test.jpg",&jpegSize); //
  if (jpegImage==0) { return 0; }

  unsigned int i=0;
 //  while (1)
  {
   startTime = AmmClient_GetTickCountMicroseconds();

   if (
        AmmClient_SendFile(
                            inst,
                            "/stream/upload.php",
                            "fileToUpload",
                            "test.jpg",
                            "image/jpeg",
                            jpegImage, jpegSize,
                            //"TEST",5,
                            1
                          )
      )
         {
          fprintf(stderr,"* %u",jpegSize);
          usleep(1000000);
          char buf[2048]={0};
          unsigned int recvdSize=2047;

          if (!AmmClient_Recv(inst,buf,&recvdSize) )
             {
               fprintf(stderr,"Failed to recv.. \n");
             }

          fprintf(stderr,"Response = `%s`\n",buf);




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



 return 1;
}



int main(int argc, char *argv[])
{

  //spam();
  stream();

  return 0;
}
