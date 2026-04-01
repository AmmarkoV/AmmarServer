#include "openssl_server.h"

#if USE_OPENSSL
// --------------------------------------------
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
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
    fprintf(stderr, "ASRV_SSL_AcceptConnection: called (fd=%d sslctx=%p)\n",
            clientsock, instance ? (void*)instance->sslctx : NULL);

    if (!instance)          { fprintf(stderr, "ASRV_SSL_AcceptConnection: null instance\n"); return 0; }
    if (!transaction)       { fprintf(stderr, "ASRV_SSL_AcceptConnection: null transaction\n"); return 0; }
    if (!instance->sslctx)  { fprintf(stderr, "ASRV_SSL_AcceptConnection: sslctx is NULL\n"); return 0; }

    SSL * ssl = SSL_new(instance->sslctx);
    if (!ssl)
    {
        fprintf(stderr, "ASRV_SSL_AcceptConnection: SSL_new failed\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }

    SSL_set_fd(ssl, clientsock);
    fprintf(stderr, "ASRV_SSL_AcceptConnection: starting SSL_accept on fd=%d\n", clientsock);

    int ret = SSL_accept(ssl);
    if (ret <= 0)
    {
        int err = SSL_get_error(ssl, ret);
        fprintf(stderr, "ASRV_SSL_AcceptConnection: SSL_accept failed (SSL_get_error=%d)\n", err);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return 0;
    }

    fprintf(stderr, "ASRV_SSL_AcceptConnection: TLS handshake complete (cipher=%s)\n",
            SSL_get_cipher(ssl));
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


int ASRV_SSL_BindHTTPSSocket(struct AmmServer_Instance * instance)
{
    fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: called (instance=%p sslAvailable=%d sslserversock=%d HTTPS_PORT=%u)\n",
            (void*)instance,
            instance ? instance->sslAvailable : -1,
            instance ? instance->sslserversock : -1,
            instance ? instance->settings.HTTPS_PORT : 0);

    if (!instance)              { fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: null instance\n"); return 0; }
    if (!instance->sslAvailable){ fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: sslAvailable=0, SSL context not ready\n"); return 0; }
    if (instance->settings.HTTPS_PORT <= 0) { fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: HTTPS_PORT not set\n"); return 0; }
    if (instance->sslserversock >= 0) { fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: already bound (fd=%d)\n", instance->sslserversock); return 1; }

    int sslsock = socket(AF_INET, SOCK_STREAM, 0);
    if (sslsock < 0)
    {
        perror("ASRV_SSL_BindHTTPSSocket: socket()");
        return 0;
    }
    fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: socket fd=%d\n", sslsock);

    int yes = 1;
    setsockopt(sslsock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    struct sockaddr_in ssl_server;
    memset(&ssl_server, 0, sizeof(ssl_server));
    ssl_server.sin_family      = AF_INET;
    ssl_server.sin_port        = htons((uint16_t)instance->settings.HTTPS_PORT);
    ssl_server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sslsock, (struct sockaddr *)&ssl_server, sizeof(ssl_server)) < 0)
    {
        perror("ASRV_SSL_BindHTTPSSocket: bind()");
        fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: bind() failed on port %u\n", instance->settings.HTTPS_PORT);
        close(sslsock);
        return 0;
    }
    fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: bind() ok on port %u\n", instance->settings.HTTPS_PORT);

    if (listen(sslsock, 128) < 0)
    {
        perror("ASRV_SSL_BindHTTPSSocket: listen()");
        close(sslsock);
        return 0;
    }

    instance->sslserversock = sslsock;
    fprintf(stderr, "ASRV_SSL_BindHTTPSSocket: HTTPS socket ready fd=%d port=%u\n", sslsock, instance->settings.HTTPS_PORT);
    return 1;
}


void ASRV_SSL_DestroyContext(struct AmmServer_Instance * instance)
{
    if (!instance || !instance->sslctx) { return; }
    SSL_CTX_free(instance->sslctx);
    instance->sslctx    = NULL;
    instance->sslAvailable = 0;
}

#endif // USE_OPENSSL
