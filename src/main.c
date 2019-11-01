#include <stdint.h>
#include <stdio.h>

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
    const char *line = irc_read();
    if (line == 0) {
      log(LOG_ERROR, "Unable to read line from server");
      return 1;
    }

    printf("%s\n", line);
  }
}
