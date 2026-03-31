#include "openssl_server.h"

#if USE_OPENSSL
// --------------------------------------------
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
// --------------------------------------------

int ASRV_SSL_InitContext(struct AmmServer_Instance * instance)
{
    if (!instance) { return 0; }

    OPENSSL_init_ssl(0, NULL);

    SSL_CTX * ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx)
    {
        fprintf(stderr, "ASRV_SSL_InitContext: SSL_CTX_new failed\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_SINGLE_DH_USE);

    if (!instance->settings.SSL_CERT_PATH || !instance->settings.SSL_KEY_PATH)
    {
        fprintf(stderr, "ASRV_SSL_InitContext: SSL_CERT_PATH or SSL_KEY_PATH not set\n");
        SSL_CTX_free(ctx);
        return 0;
    }

    if (SSL_CTX_use_certificate_file(ctx, instance->settings.SSL_CERT_PATH, SSL_FILETYPE_PEM) <= 0)
    {
        fprintf(stderr, "ASRV_SSL_InitContext: failed to load certificate %s\n", instance->settings.SSL_CERT_PATH);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 0;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, instance->settings.SSL_KEY_PATH, SSL_FILETYPE_PEM) <= 0)
    {
        fprintf(stderr, "ASRV_SSL_InitContext: failed to load private key %s\n", instance->settings.SSL_KEY_PATH);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 0;
    }

    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "ASRV_SSL_InitContext: certificate and private key do not match\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 0;
    }

    instance->sslctx = ctx;
    instance->sslAvailable = 1;
    fprintf(stderr, "ASRV_SSL_InitContext: TLS context ready (cert=%s)\n", instance->settings.SSL_CERT_PATH);
    return 1;
}


int ASRV_SSL_AcceptConnection(struct AmmServer_Instance * instance,
                               struct HTTPTransaction    * transaction,
                               int                        clientsock)
{
    if (!instance || !transaction || !instance->sslctx) { return 0; }

    SSL * ssl = SSL_new(instance->sslctx);
    if (!ssl)
    {
        fprintf(stderr, "ASRV_SSL_AcceptConnection: SSL_new failed\n");
        return 0;
    }

    SSL_set_fd(ssl, clientsock);

    if (SSL_accept(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return 0;
    }

    transaction->ssl = ssl;
    return 1;
}


void ASRV_SSL_ShutdownConnection(struct HTTPTransaction * transaction)
{
    if (!transaction || !transaction->ssl) { return; }
    /* Send close_notify; do not wait for peer reply to avoid hanging on broken clients. */
    SSL_shutdown(transaction->ssl);
    SSL_free(transaction->ssl);
    transaction->ssl = NULL;
}


void ASRV_SSL_DestroyContext(struct AmmServer_Instance * instance)
{
    if (!instance || !instance->sslctx) { return; }
    SSL_CTX_free(instance->sslctx);
    instance->sslctx    = NULL;
    instance->sslAvailable = 0;
}

#endif // USE_OPENSSL
