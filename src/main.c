#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "resources/resources.h"
#include "logging/logging.h"
#include "irc/irc.h"
#include "tls/tls.h"

int main(int argc, const char *argv[]) {
  tls_initialize();

  bool connected = irc_connect("irc.blesstherains.africa", 6697, "test-bot", "test-bot", "hello");
  if (!connected) {
    log(LOG_ERROR, "Unable to connect to the server");
    return 1;
  }

  irc_join("#bot-test");

  while (true) {
    irc_message_t *message = irc_read();
    if (message == 0) {
      log(LOG_ERROR, "Unable to read message from server");
      return 1;
    }

    // Ignore anything but regular messages
    if (strcmp(message->type, "PRIVMSG") != 0)
      continue;

    log(LOG_DEBUG, "Got message '%s' from '%s' in '%s'", message->message, message->sender, message->target);
  }
}
