#ifndef IRC_H
#define IRC_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>

#define IRC_MESSAGE_MAX_SIZE 1024
#define IRC_MESSAGE_TIMEOUT -1

bool irc_connect(const char *server, uint16_t port, const char *user, const char *nick, const char *gecos);

void irc_disconnect();

void irc_join(const char *channel);

void irc_leave(const char *channel);

void irc_write(const char *format, ...);

void irc_send(const char *channel, const char *message);

const char *irc_read();

#endif
