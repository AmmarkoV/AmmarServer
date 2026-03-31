#include "networkAbstraction.h"
#include "openssl_server.h"
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
#if USE_OPENSSL
 #include <openssl/ssl.h>
#endif // USE_OPENSSL
// --------------------------------------------


int ASRV_StartSession(
                      struct AmmServer_Instance * instance,
                      struct HTTPTransaction * transaction
                     )
{
  #if USE_OPENSSL
   transaction->ssl = NULL;
  #endif
  return 0;
}

int ASRV_StopSession(
                      struct AmmServer_Instance * instance,
                      struct HTTPTransaction * transaction
                     )
{
  #if USE_OPENSSL
   ASRV_SSL_ShutdownConnection(transaction);
  #endif
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
  int opres;
  #if USE_OPENSSL
  if (transaction->ssl != NULL)
    {
      opres = SSL_write(transaction->ssl, buf, (int)len);
    } else
  #endif // USE_OPENSSL
    {
      opres = send(transaction->clientSock, buf, len, flags);
    }

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
  ssize_t opres;
  #if USE_OPENSSL
  if (transaction->ssl != NULL)
    {
      opres = SSL_read(transaction->ssl, buf, (int)len);
    } else
  #endif // USE_OPENSSL
    {
      opres = recv(transaction->clientSock, buf, len, flags);
    }

  if (opres>0)
       {
         instance->statistics.totalDownloadKB+=(unsigned long) opres/1024;
         instance->statistics.totalDownloadBytes+=(unsigned long)opres;
       }
  return opres;
}
