#include <string.h>

#include "../logging/logging.h"

#include "irc.h"

irc_t *irc_connect(char *hostname, uint16_t port, char *user, char *nick, char *gecos) {
  irc_t *irc = malloc(sizeof(irc_t));
  if (irc == 0) {
    log(LOG_ERROR, "Unable to allocate IRC structure");
    return 0;
  }
  memset(irc, 0, sizeof(irc_t));

  irc->hostname = hostname;
  irc->port = port;
  irc->user = user;
  irc->nick = nick;
  irc->gecos = gecos;

  irc->tls = tls_connect(hostname, port);
  if (irc->tls == 0) {
    log(LOG_ERROR, "Unable to connect to server '%s'", hostname);
    free(irc);
    return 0;
  }

  log(LOG_DEBUG, "Successfully connected to server '%s:%d'", hostname, port);

  log(LOG_DEBUG, "Registering as '%s' and gecos '%s'", user, gecos);
  irc_write(irc, "USER %s %s %s :%s\r\n", user, user, user, gecos);

  log(LOG_DEBUG, "Using nickname '%s'", nick);
  irc_write(irc, "NICK %s\r\n", nick);

  return irc;
}

void irc_write(irc_t *irc, const char *format, ...) {
  va_list arguments;

  va_start(arguments, format);
  ssize_t messageLength = vsnprintf(NULL, 0, format, arguments);
  va_end(arguments);

  if (messageLength < 0) {
    log(LOG_ERROR, "Unable to get message length");
    return;
  }

  char *message = malloc(messageLength + 1);
  if (message == 0) {
    log(LOG_ERROR, "Unable to allocate message");
    return;
  }

  va_start(arguments, format);
  vsnprintf(message, messageLength + 1, format, arguments);
  va_end(arguments);

  tls_write(irc->tls, message, messageLength);
  free(message);
}

void irc_pong(irc_t *irc, const char *server, const char *server2) {
  irc_write(irc, "PONG %s %s", server, server2);
}

irc_message_t *irc_read(irc_t *irc) {
  irc_message_t *message = malloc(sizeof(irc_message_t));
  if (message == 0) {
    log(LOG_ERROR, "Unable to allocate message");
    return 0;
  }
  memset(message, 0, sizeof(irc_message_t));

  char *buffer = tls_readLine(irc->tls, IRC_MESSAGE_TIMEOUT, IRC_MESSAGE_MAX_SIZE);
  if (buffer == 0) {
    log(LOG_ERROR, "Unable to read buffer");
    free(message);
    return 0;
  }

  size_t messageLength = strlen(buffer);
  size_t start = 0;
  for (size_t i = 0; i < messageLength + 1; i++) {
    if (buffer[i] == ' ' || buffer[i] == 0) {
      size_t valueLength = i - start;
      if (message->sender == 0) {
        // The start of the username is always ':'
        start++;
        if (valueLength > 1)
          valueLength--;

        // The username ends with '!'
        for (size_t j = 0; j < valueLength; j++) {
          if (buffer[start + j] == '!') {
            valueLength = j;
            break;
          }
        }

        message->sender = malloc(sizeof(char) * (valueLength + 1));
        memcpy(message->sender, buffer + start, (valueLength));
        message->sender[valueLength] = 0;
      } else if (message->type == 0) {
        message->type = malloc(sizeof(char) * (valueLength + 1));
        memcpy(message->type, buffer + start, (valueLength));
        message->type[valueLength] = 0;
      } else if (message->target == 0) {
        message->target = malloc(sizeof(char) * (valueLength + 1));
        memcpy(message->target, buffer + start, (valueLength));
        message->target[valueLength] = 0;
      } else if (message->message == 0) {
        // The start of the message is always ':'
        start++;
        // The message occupies the rest of the string
        valueLength = messageLength - start;

        message->message = malloc(sizeof(char) * (valueLength + 1));
        memcpy(message->message, buffer + start, (valueLength));
        message->message[valueLength] = 0;
        break;
      }

      start = i + 1;
    }
  }

  free(buffer);
  return message;
}

void irc_join(irc_t *irc, const char *channel) {
  irc_write(irc, "JOIN %s\r\n", channel);
}

void irc_free(irc_t *irc) {
  tls_free(irc->tls);
  free(irc);
}

void irc_freeMessage(irc_message_t *message) {
  if (message->sender != 0)
    free(message->sender);
  if (message->type != 0)
    free(message->type);
  if (message->target != 0)
    free(message->target);
  if (message->message != 0)
    free(message->message);

  free(message);
}
