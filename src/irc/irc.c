#include <string.h>

#include "../logging/logging.h"
#include "../tls/tls.h"

#include "irc.h"

tls_t *irc_tls = 0;

bool irc_connect(const char *hostname, uint16_t port, const char *user, const char *nick, const char *gecos) {
  tls_t *tls = tls_connect(hostname, port);
  if (tls == 0) {
    log(LOG_ERROR, "Unable to connect to server '%s'", hostname);
    return false;
  }

  log(LOG_DEBUG, "Successfully connected to server '%s:%d'", hostname, port);
  irc_tls = tls;

  log(LOG_DEBUG, "Registering as '%s' and gecos '%s'", user, gecos);
  irc_write("USER %s %s %s :%s\r\n", user, user, user, gecos);

  log(LOG_DEBUG, "Using nickname '%s'", nick);
  irc_write("NICK %s\r\n", nick);

  return true;
}

void irc_write(const char *format, ...) {
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

  tls_write(irc_tls, message, messageLength);
  free(message);
}

void irc_pong(const char *server, const char *server2) {
  irc_write("PONG %s %s", server, server2);
}

irc_message_t *irc_read() {
  irc_message_t *message = malloc(sizeof(irc_message_t));
  if (message == 0) {
    log(LOG_ERROR, "Unable to allocate message");
    return 0;
  }
  memset(message, 0, sizeof(irc_message_t));

  char *buffer = tls_readLine(irc_tls, IRC_MESSAGE_TIMEOUT, IRC_MESSAGE_MAX_SIZE);
  if (buffer == 0) {
    log(LOG_ERROR, "Unable to read buffer");
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
        if (valueLength > 1)
          valueLength--;

        message->message = malloc(sizeof(char) * (valueLength + 1));
        memcpy(message->message, buffer + start, (valueLength));
        message->message[valueLength] = 0;
      }

      start = i + 1;
    }
  }

  return message;
}

void irc_join(const char *channel) {
  irc_write("JOIN %s\r\n", channel);
}
