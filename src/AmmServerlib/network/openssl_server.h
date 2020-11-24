#ifndef OPENSSL_SERVER_H_INCLUDED
#define OPENSSL_SERVER_H_INCLUDED

#include "../AmmServerlib.h"

#if USE_OPENSSL
  int InitializeSSL();
  int sslAcceptStuff(struct AmmServer_Instance * instance,int newsockfd);
#endif // USE_OPENSSL

int startOpenSSLServer();

#endif // OPENSSL_SERVER_H_INCLUDED
