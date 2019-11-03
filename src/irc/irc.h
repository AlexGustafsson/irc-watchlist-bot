#ifndef IRC_H
#define IRC_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>

#include "../tls/tls.h"

#define IRC_MESSAGE_MAX_SIZE 1024
#define IRC_MESSAGE_TIMEOUT -1

typedef struct {
  char *hostname;
  uint16_t port;
  char *user;
  char *nick;
  char *gecos;

  tls_t *tls;
} irc_t;

typedef struct {
  char *sender;
  char *type;
  char *target;
  char *message;
} irc_message_t;

irc_t *irc_connect(char *hostname, uint16_t port, char *user, char *nick, char *gecos);

void irc_disconnect(irc_t *irc);

void irc_join(irc_t *irc, const char *channel);

void irc_write(irc_t *irc, const char *format, ...);

void irc_send(irc_t *irc, const char *channel, const char *message);

void irc_pong(irc_t *irc, const char *server, const char *server2);

irc_message_t *irc_read(irc_t *irc);

void irc_free(irc_t *irc);
void irc_freeMessage(irc_message_t *message);

#endif
