#ifndef AMMCLIENT_H_INCLUDED
#define AMMCLIENT_H_INCLUDED

struct AmmClient_Instance
{
  int clientSocket;
  int connectionOK;
  unsigned int failedReconnections;

  char ip[32];
  int port;

  void * internals;
};


unsigned long AmmClient_GetTickCountMicroseconds();
unsigned long AmmClient_GetTickCountMilliseconds();


int AmmClient_CheckConnection(struct AmmClient_Instance * instance);

int AmmClient_Recv(struct AmmClient_Instance * instance,
                   char * buffer ,
                   unsigned int * bufferSize
                  );


int AmmClient_Send(struct AmmClient_Instance * instance,
                   const char * request ,
                   unsigned int requestSize,
                   int keepAlive
                  );


struct AmmClient_Instance * AmmClient_Initialize(
                                                  const char * ip ,
                                                  unsigned int port
                                                );


int AmmClient_Close(struct AmmClient_Instance * instance);

#endif // AMMCLIENT_H_INCLUDED
