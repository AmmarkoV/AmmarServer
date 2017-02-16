#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "AmmClient.h"


struct AmmClient_Internals
{
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
};

int AmmClient_Recv(struct AmmClient_Instance * instance,
                   char * buffer ,
                   unsigned int * bufferSize
                  )
{
  struct AmmClient_Internals * ctx = (struct AmmClient_Internals *) instance->internals;

  if (ctx!=0)
  {
   /*---- Read the message from the server into the buffer ----*/
   *bufferSize = recv(instance->clientSocket, buffer, bufferSize, 0);

   /*---- Print the received message ----*/
   printf("Data received: %s\n",buffer);
   return 1;
  }

 return 0;
}

int AmmClient_Send(struct AmmClient_Instance * instance,
                   const char * request ,
                   unsigned int requestSize,
                   int keepAlive
                  )
{
 struct AmmClient_Internals * ctx = (struct AmmClient_Internals *) instance->internals;

  if (ctx!=0)
  {
   send(instance->clientSocket,request,requestSize,0);

   return 1;
  }

 return 0;
}



struct AmmClient_Instance * AmmClient_Initialize(
                                                  const char * ip ,
                                                  unsigned int port
                                                )
{
  struct AmmClient_Instance * instance = (struct AmmClient_Instance *) malloc(sizeof(struct AmmClient_Instance));


  if (instance!=0)
  {
   snprintf(instance->ip,64,"%s",ip);
   instance->port = port;

   instance->internals = (void *) malloc(sizeof (struct AmmClient_Internals));

   if (instance->internals!=0)
   {
    struct AmmClient_Internals * ctx = (struct AmmClient_Internals *) instance->internals;


   /*---- Create the socket. The three arguments are: ----*/
   /* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
   instance->clientSocket = socket(PF_INET, SOCK_STREAM, 0);

   /*---- Configure settings of the server address struct ----*/
   /* Address family = Internet */
   ctx->serverAddr.sin_family = AF_INET;
   /* Set port number, using htons function to use proper byte order */
   ctx->serverAddr.sin_port = htons(instance->port);
   /* Set IP address to localhost */
   ctx->serverAddr.sin_addr.s_addr = inet_addr(instance->ip);
   /* Set all bits of the padding field to 0 */
   memset(ctx->serverAddr.sin_zero, '\0', sizeof ctx->serverAddr.sin_zero);

   /*---- Connect the socket to the server using the address struct ----*/
   ctx->addr_size = sizeof ctx->serverAddr;
   connect(instance->clientSocket, (struct sockaddr *) &ctx->serverAddr, ctx->addr_size);

  } else
  {
   fprintf(stderr,"Could not allocate internals.. \n");
  }




  }

  return instance;
}




int AmmClient_Close(struct AmmClient_Instance * instance)
{
 if (instance->clientSocket!=0)
   { free(instance->clientSocket); }

 free(instance);

 return 1;
}
