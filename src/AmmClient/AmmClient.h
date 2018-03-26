#ifndef AMMCLIENT_H_INCLUDED
#define AMMCLIENT_H_INCLUDED


#ifdef __cplusplus
extern "C"
{
#endif

struct AmmClient_Instance
{
  int clientSocket;
  int connectionOK;
  int socketOK;
  unsigned int failedReconnections;

  int socketTimeoutSeconds;

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
                                                  unsigned int port,
                                                  unsigned int socketTimeoutSeconds
                                                );


int AmmClient_Close(struct AmmClient_Instance * instance);

#ifdef __cplusplus
}
#endif

#endif // AMMCLIENT_H_INCLUDED
