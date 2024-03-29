#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "irc/irc.h"
#include "logging/logging.h"
#include "resources/resources.h"
#include "tls/tls.h"

#include "main.h"

static irc_t *main_irc = 0;
static irc_message_t *main_message = 0;

int main(int argc, const char *argv[]) {
  // Setup signal handling for main process
  signal(SIGINT, main_handleSignalSIGINT);
  signal(SIGTERM, main_handleSignalSIGTERM);

  char *hostname = getenv("IRC_SERVER");
  char *portString = getenv("IRC_PORT");
  uint16_t port = atoi(portString);
  char *user = getenv("IRC_USER");
  char *nick = getenv("IRC_NICK");
  char *gecos = getenv("IRC_GECOS");
  char *channel = getenv("IRC_CHANNEL");

  char *logLevel = getenv("LOGGING_LEVEL");
  if (logLevel != 0) {
    if (strcasecmp(logLevel, "debug") == 0)
      LOGGING_LEVEL = LOG_DEBUG;
    else if (strcasecmp(logLevel, "info") == 0)
      LOGGING_LEVEL = LOG_INFO;
    else if (strcasecmp(logLevel, "notice") == 0)
      LOGGING_LEVEL = LOG_NOTICE;
    else if (strcasecmp(logLevel, "warning") == 0)
      LOGGING_LEVEL = LOG_WARNING;
    else if (strcasecmp(logLevel, "error") == 0)
      LOGGING_LEVEL = LOG_ERROR;
    else if (strcasecmp(logLevel, "critical") == 0)
      LOGGING_LEVEL = LOG_CRITICAL;
    else if (strcasecmp(logLevel, "alert") == 0)
      LOGGING_LEVEL = LOG_ALERT;
    else if (strcasecmp(logLevel, "emergency") == 0)
      LOGGING_LEVEL = LOG_EMERGENCY;
  }

  tls_initialize();

  main_irc = irc_connect(hostname, port, user, nick, gecos);
  if (main_irc == 0) {
    log(LOG_ERROR, "Unable to connect to the server");
    return 1;
  }

  irc_join(main_irc, channel);

  while (true) {
    main_message = irc_read(main_irc);
    if (main_message == 0) {
      log(LOG_ERROR, "Unable to read message from server");
      break;
    }

    log(LOG_DEBUG, "Got message '%s' (type '%s') from '%s' in '%s'", main_message->message, main_message->type, main_message->sender, main_message->target);

    if (strcmp(main_message->type, "PING") == 0) {
      irc_write(main_irc, "PONG :%s\r\n", main_message->message);

      irc_freeMessage(main_message);
      main_message = 0;
      continue;
    }
    if (strcmp(main_message->type, "PRIVMSG") != 0) {
      irc_freeMessage(main_message);
      main_message = 0;
      continue;
    }

    if (strcasecmp(main_message->message, "watchlist-bot: help") == 0)
      main_handleHelp();
    else
      main_handleWatchlist();

    irc_freeMessage(main_message);
    main_message = 0;
  }

  irc_free(main_irc);
  main_irc = 0;
  log(LOG_DEBUG, "Everything freed, closing");
}

void main_handleHelp() {
  irc_write(main_irc, "PRIVMSG %s :%s\r\n", main_message->target, "I keep track of words used in nations' watchlists. I currently handle English words watched by NSA and USA in general.");
}

void main_handleWatchlist() {
  size_t messageLength = strlen(main_message->message);
  char *message = main_message->message;
  size_t start = 0;
  size_t occurances[RESOURCES_DATA_SOURCES] = {0};
  for (size_t i = 0; i < messageLength + 1; i++) {
    if (message[i] == ' ' || message[i] == 0) {
      size_t wordLength = i - start;

      char *word = malloc(sizeof(char) * (wordLength + 1));
      memcpy(word, message + start, (wordLength));
      word[wordLength] = 0;

      resources_countWord(word, occurances);

      free(word);
      start = i + 1;
    }
  }

  uint8_t bestMatch = resources_bestMatch(occurances);

  switch (bestMatch) {
  case COUNTRY_USA:
    irc_write(main_irc, "PRIVMSG %s :%s\r\n", main_message->target, "USA is watching 👀");
    break;
  case COUNTRY_USA_NSA:
    irc_write(main_irc, "PRIVMSG %s :%s\r\n", main_message->target, "NSA is watching 👀");
    break;
  }
}

// Handle SIGINT (CTRL + C)
void main_handleSignalSIGINT(int signalNumber) {
  // Disable handling of SIGCHILD
  signal(SIGCHLD, main_emptySignalHandler);

  log(LOG_INFO, "Got SIGINT - exiting cleanly");

  if (main_message != 0)
    irc_freeMessage(main_message);
  if (main_irc != 0)
    irc_free(main_irc);

  exit(0);
}

// Handle SIGTERM (kill etc.)
void main_handleSignalSIGTERM(int signalNumber) {
  // Disable handling of SIGCHILD
  signal(SIGCHLD, main_emptySignalHandler);

  log(LOG_INFO, "Got SIGTERM - exiting cleanly");

  if (main_message != 0)
    irc_freeMessage(main_message);
  if (main_irc != 0)
    irc_free(main_irc);

  exit(0);
}

void main_emptySignalHandler(int signalNumber) {
  // Do nothing
}
