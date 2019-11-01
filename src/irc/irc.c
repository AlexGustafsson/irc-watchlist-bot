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

const char *irc_read() {
  return tls_readLine(irc_tls, IRC_MESSAGE_TIMEOUT, IRC_MESSAGE_MAX_SIZE);
}

void irc_join(const char *channel) {
  irc_write("JOIN %s\r\n", channel);
}
