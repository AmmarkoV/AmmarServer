#include "networkAbstraction.h"
// --------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// --------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
// --------------------------------------------

//Counters for performance , these should  be put inside the the server instance so this is work to do in the future..

int ASRV_StartSession(
                      struct AmmServer_Instance * instance,
                      struct HTTPTransaction * transaction
                     )
{

  return 0;
}

int ASRV_StopSession(
                      struct AmmServer_Instance * instance,
                      struct HTTPTransaction * transaction
                     )
{

  return 0;
}




int ASRV_Send(
              struct AmmServer_Instance * instance,
              struct HTTPTransaction * transaction,
              const void *buf,
              size_t len,
              int flags
              )
{
  int opres=send(transaction->clientSock,buf,len,flags);
  if (opres>0)
       {
        instance->statistics.totalUploadKB+=(unsigned long) opres/1024;
        instance->statistics.totalUploadBytes+=(unsigned long)opres;
       }
  return opres;
}


ssize_t ASRV_Recv(
                  struct AmmServer_Instance * instance,
                  struct HTTPTransaction * transaction,
                  void *buf, size_t len, int flags)
{
  ssize_t opres=recv(transaction->clientSock,buf,len,flags);
  if (opres>0)
       {
         instance->statistics.totalDownloadKB+=(unsigned long) opres/1024;
         instance->statistics.totalDownloadBytes+=(unsigned long)opres;
       }
  return opres;
}
