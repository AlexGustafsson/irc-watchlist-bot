#ifndef IRC_H
#define IRC_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>

#define IRC_MESSAGE_MAX_SIZE 1024
#define IRC_MESSAGE_TIMEOUT -1

typedef struct {
  char *sender;
  char *type;
  char *target;
  char *message;
} irc_message_t;

bool irc_connect(const char *server, uint16_t port, const char *user, const char *nick, const char *gecos);

void irc_disconnect();

void irc_join(const char *channel);

void irc_leave(const char *channel);

void irc_write(const char *format, ...);

void irc_send(const char *channel, const char *message);

void irc_pong(const char *server, const char *server2);

irc_message_t *irc_read();

#endif
