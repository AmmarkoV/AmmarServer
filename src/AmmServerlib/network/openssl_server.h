#ifndef OPENSSL_SERVER_H_INCLUDED
#define OPENSSL_SERVER_H_INCLUDED

#include "../AmmServerlib.h"

#if USE_OPENSSL
  /** Initialize the SSL_CTX once at server startup. Cert/key paths come from instance->settings. */
  int  ASRV_SSL_InitContext(struct AmmServer_Instance * instance);

  /** Per-connection TLS handshake, called in the worker thread after accept().
      On success sets transaction->ssl and returns 1. On failure returns 0. */
  int  ASRV_SSL_AcceptConnection(struct AmmServer_Instance * instance,
                                  struct HTTPTransaction    * transaction,
                                  int                        clientsock);

  /** Send TLS close_notify and free the SSL object. Idempotent (safe to call twice). */
  void ASRV_SSL_ShutdownConnection(struct HTTPTransaction * transaction);

  /** Bind and listen on the HTTPS port. Call after ASRV_SSL_InitContext succeeds. */
  int  ASRV_SSL_BindHTTPSSocket(struct AmmServer_Instance * instance);

  /** Free the SSL_CTX at server shutdown. */
  void ASRV_SSL_DestroyContext(struct AmmServer_Instance * instance);
#endif // USE_OPENSSL

#endif // OPENSSL_SERVER_H_INCLUDED
