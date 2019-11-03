#ifndef TLS_H
#define TLS_H

#include <stdbool.h>
#include <stdint.h>

#include <openssl/ssl.h>

#define READ_FLAGS_NONE 0
#define READ_FLAGS_PEEK MSG_PEEK

// See:
// https://wiki.mozilla.org/Security/Server_Side_TLS
// https://www.openssl.org/docs/man1.1.1/man1/ciphers.html
#define TLS_DEFAULT_ELLIPTIC_CURVES "P-256:P-384:X25519"
#define TLS_DEFAULT_TLS_1_2_CIPHER_SUITE "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384"
#define TLS_DEFAULT_TLS_1_3_CIPHER_SUITE "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256"

#define TLS_POLL_STATUS_FAILED -1
#define TLS_POLL_STATUS_NOT_AVAILABLE 0
#define TLS_POLL_STATUS_AVAILABLE 1

typedef struct {
  int socketId;
  SSL *ssl;
  char *buffer;
  size_t bufferSize;
} tls_t;

bool tls_initialize();
tls_t *tls_connect(const char *hostname, uint16_t port);
bool tls_setNonBlocking(tls_t *tls);
size_t tls_read(tls_t *tls, char **buffer, size_t bytesToRead, int flags);
ssize_t tls_getAvailableBytes(tls_t *tls);
int tls_pollForData(tls_t *tls, int timeout);
size_t tls_write(tls_t *tls, const char *buffer, size_t bufferSize);
void tls_disconnect(tls_t *tls);
char *tls_readLine(tls_t *tls, int timeout, size_t maxBytes);
void tls_free(tls_t *tls);

#endif
