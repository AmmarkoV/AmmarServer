#ifndef OPENSSL_SERVER_H_INCLUDED
#define OPENSSL_SERVER_H_INCLUDED


#if USE_OPENSSL
int InitializeSSL();
#endif // USE_OPENSSL

int startOpenSSLServer();

#endif // OPENSSL_SERVER_H_INCLUDED
