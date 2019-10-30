#ifndef IRC_H
#define IRC_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>

// See:
// https://wiki.mozilla.org/Security/Server_Side_TLS
// https://www.openssl.org/docs/man1.1.1/man1/ciphers.html
#define IRC_TLS_DEFAULT_ELLIPTIC_CURVES "P-256:P-384:X25519"
#define IRC_TLS_DEFAULT_TLS_1_2_CIPHER_SUITE "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384"
#define IRC_TLS_DEFAULT_TLS_1_3_CIPHER_SUITE "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256"

#define IRC_MESSAGE_MAX_SIZE 1024
#define IRC_MESSAGE_TIMEOUT -1

#define READ_FLAGS_NONE 0
#define READ_FLAGS_PEEK MSG_PEEK

bool irc_initialize();

bool irc_connect(const char *server, uint16_t port, const char *user, const char *nick, const char *gecos);

void irc_disconnect();

void irc_join(const char *channel);

void irc_leave(const char *channel);

void irc_write(const char *format, ...);

void irc_send(const char *channel, const char *message);

const char *irc_read();

#endif
