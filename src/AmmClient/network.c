
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "network.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>


struct AmmClient_Internals
{
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
};



int AmmClient_ReconnectInternal(
                         struct AmmClient_Instance * instance ,
                         int triggerCheck
                       )
{
    struct AmmClient_Internals * ctx = (struct AmmClient_Internals *) instance->internals;

   /*---- Create the socket. The three arguments are: ----*/
   /* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
   instance->clientSocket = socket(PF_INET, SOCK_STREAM, 0);


    struct timeval timeout;
    timeout.tv_sec = (unsigned int) 5; timeout.tv_usec = 0;
    if (setsockopt (instance->clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
        { fprintf(stderr,"Warning : Could not set socket Receive timeout \n"); }

    timeout.tv_sec = (unsigned int) 5; timeout.tv_usec = 0;
    if (setsockopt (instance->clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
        { fprintf(stderr,"Warning : Could not set socket Send timeout \n"); }


   /*---- Configure settings of the server address struct ----*/
   /* Address family = Internet */
   ctx->serverAddr.sin_family = AF_INET;
   /* Set port number, using htons function to use proper byte order */
   ctx->serverAddr.sin_port = htons(instance->port);
   /* Set IP address to localhost */
   ctx->serverAddr.sin_addr.s_addr = inet_addr(instance->ip);
   /* Set all bits of the padding field to 0 */
   memset(ctx->serverAddr.sin_zero, '\0', sizeof ctx->serverAddr.sin_zero);
   ctx->addr_size = sizeof ctx->serverAddr;

   /*---- Connect the socket to the server using the address struct ----*/
   instance->connectionOK=0;
    //Check connection for the first time..
    if (triggerCheck)
       { return AmmClient_CheckConnection(instance); }
 return 1;
}


int AmmClient_CheckConnectionInternal(struct AmmClient_Instance * instance)
{
 if (instance->connectionOK)  { return 1; }
 struct AmmClient_Internals * ctx = (struct AmmClient_Internals *) instance->internals;

  if (ctx!=0)
  {
   if (instance->failedReconnections%10==9)
     {
       AmmClient_ReconnectInternal( instance , 0 );
     }


   if ( connect(instance->clientSocket, (struct sockaddr *) &ctx->serverAddr, ctx->addr_size) < 0 )
     {
       instance->connectionOK=0;
       fprintf(stderr,"Reconnection attempt %u failed .. \n" , instance->failedReconnections);
       ++instance->failedReconnections;
       usleep(100000);
     } else
     {
      fprintf(stderr,"Reestablished connection.. \n");
      instance->failedReconnections=0;
      instance->connectionOK=1;
     }
    return instance->connectionOK;
  }
 return 0;
}



int AmmClient_RecvInternal(struct AmmClient_Instance * instance,
                   char * buffer ,
                   unsigned int * bufferSize
                  )
{
  if (!AmmClient_CheckConnection(instance)) { return 0; }
  struct AmmClient_Internals * ctx = (struct AmmClient_Internals *) instance->internals;

  if (ctx!=0)
  {
   /*---- Read the message from the server into the buffer ----*/
   int result = recv(instance->clientSocket, buffer, *bufferSize, 0);

   /*---- Print the received message ----*/
   //printf("Data received: %s\n",buffer);


   if (result < 0 ) {
                     fprintf(stderr,"Failed to Recv error : %u\n",errno);
                     instance->connectionOK=0;
                     return 0;
                    } else
                    {
                      fprintf(stderr,"Recvd %u/%u bytes\n",result,*bufferSize);
                      *bufferSize = result;
                    }

   return 1;
  }

 return 0;
}

int AmmClient_SendInternal(struct AmmClient_Instance * instance,
                   const char * request ,
                   unsigned int requestSize,
                   int keepAlive
                  )
{
  if (!AmmClient_CheckConnection(instance)) { return 0; }
  struct AmmClient_Internals * ctx = (struct AmmClient_Internals *) instance->internals;

  if (ctx!=0)
  {
   int result = send(instance->clientSocket,request,requestSize,MSG_WAITALL|MSG_NOSIGNAL);

   if (result < 0 ) {
                     fprintf(stderr,"Failed to Send error : %u\n",errno);
                     instance->connectionOK=0;
                     return 0;
                    } else
                    { fprintf(stderr,"Sent %u/%u bytes\n",result,requestSize); }

   return 1;
  }

 return 0;
}




struct AmmClient_Instance * AmmClient_InitializeInternal(
                                                  const char * ip ,
                                                  unsigned int port
                                                )
{
  struct AmmClient_Instance * instance = (struct AmmClient_Instance *) malloc(sizeof(struct AmmClient_Instance));
  memset(instance,0,sizeof(struct AmmClient_Instance));

  if (instance!=0)
  {
   snprintf(instance->ip,32,"%s",ip);
   instance->port = port;

   instance->internals = (void *) malloc(sizeof (struct AmmClient_Internals));

   if (instance->internals!=0)
   {
    AmmClient_ReconnectInternal(instance,1);
  } else
  {
   fprintf(stderr,"Could not allocate internals.. \n");
  }




  }

  return instance;
}




int AmmClient_CloseInternal(struct AmmClient_Instance * instance)
{
 if (instance->clientSocket!=0)
   {
     shutdown(instance->clientSocket,SHUT_RDWR);
     free(instance->internals);
   }

 free(instance);

 return 1;
}
