#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "../logging/logging.h"

#include "irc.h"

static SSL_CTX *irc_sslContext = 0;
static SSL *irc_ssl = 0;
static int irc_socketId = -1;

bool irc_setNonBlocking(int socketId);
size_t irc_readSSLBytes(SSL *ssl, char **buffer, size_t bytesToRead, int flags);
ssize_t irc_getAvailableBytes(SSL *ssl);
bool irc_pollForData(SSL *ssl, int socketId, int timeout);
size_t irc_writeSSLBytes(SSL *ssl, const char *buffer, size_t bufferSize);

bool irc_initialize() {
  const SSL_METHOD *method = TLS_method();
  SSL_CTX *sslContext = SSL_CTX_new(method);
  if (sslContext == 0) {
    log(LOG_ERROR, "Unable to instantiate SSL context");
    return false;
  }

  // Only allow TLS 1.2 and above (TLS 1.3)
  SSL_CTX_set_min_proto_version(sslContext, TLS1_2_VERSION);
  SSL_CTX_set1_curves_list(sslContext, IRC_TLS_DEFAULT_ELLIPTIC_CURVES);
  // Set the cipher suite to use for TLS 1.2
  SSL_CTX_set_cipher_list(sslContext, IRC_TLS_DEFAULT_TLS_1_2_CIPHER_SUITE);
  // Set the cipher suite to use for TLS 1.3
  SSL_CTX_set_ciphersuites(sslContext, IRC_TLS_DEFAULT_TLS_1_3_CIPHER_SUITE);

  irc_sslContext = sslContext;
  return true;
}

bool irc_connect(const char *server, uint16_t port, const char *user, const char *nick, const char *gecos) {
  // Open a IPv4 TCP socket
  int socketId = socket(AF_INET, SOCK_STREAM, 0);
  if (socketId == -1) {
    log(LOG_ERROR, "Unable to create a socket");
    return false;
  }

  if (!irc_setNonBlocking(socketId)) {
    return false;
  }

  struct hostent *hostent = gethostbyname(server);
  if (hostent == 0) {
    log(LOG_ERROR, "Unable to get host by name for server");
    return false;
  }

  struct in_addr address = *(struct in_addr *)*(hostent->h_addr_list);

  struct sockaddr_in socketAddress;
  socketAddress.sin_addr.s_addr = address.s_addr;
  socketAddress.sin_family = AF_INET;
  socketAddress.sin_port = htons(port);

  if (connect(socketId, (struct sockaddr *)&socketAddress, sizeof(struct sockaddr_in)) == 0) {
    log(LOG_ERROR, "Unable to connect to server: %s", strerror(errno));
    return false;
  }

  SSL *ssl = SSL_new(irc_sslContext);
  if (ssl == 0) {
    log(LOG_ERROR, "Unable to instantiate SSL object");
    return false;
  }

  SSL_set_fd(ssl, socketId);
  if (SSL_set_tlsext_host_name(ssl, server) != 1) {
    log(LOG_ERROR, "Unable to set the server's hostname");
    SSL_free(ssl);
    return false;
  }

  // Set up structures necessary for polling
  struct pollfd readDescriptors[1];
  memset(readDescriptors, 0, sizeof(struct pollfd));
  readDescriptors[0].fd = socketId;
  readDescriptors[0].events = POLLIN;

  struct pollfd writeDescriptors[1];
  memset(writeDescriptors, 0, sizeof(struct pollfd));
  writeDescriptors[0].fd = socketId;
  writeDescriptors[0].events = POLLOUT;

  while (true) {
    int status = SSL_connect(ssl);
    if (status == 0) {
      log(LOG_ERROR, "Unable to connect to server");
      SSL_free(ssl);

      return false;
    } else if (status < 0) {
      int error = SSL_get_error(ssl, status);
      if (error == SSL_ERROR_WANT_READ) {
        log(LOG_DEBUG, "Waiting for TLS connection to be readable");
        // Wait for the connection to be ready to read
        int status = poll(readDescriptors, 1, -1);
        if (status < -1) {
          log(LOG_ERROR, "Could not wait for connection to be readable");
          SSL_free(ssl);

          return false;
        }
      } else if (error == SSL_ERROR_WANT_WRITE) {
        log(LOG_DEBUG, "Waiting for TLS connection to be writable");
        // Wait for the connection to be ready to write
        int status = poll(writeDescriptors, 1, -1);
        if (status < -1) {
          log(LOG_ERROR, "Could not wait for connection to be writable");
          SSL_free(ssl);

          return false;
        }
      } else {
        log(LOG_ERROR, "Unable to connect to server. Got code %d", error);
        SSL_free(ssl);

        return false;
      }
    } else {
      break;
    }
  }

  log(LOG_DEBUG, "Successfully connected to server '%s:%d'", server, port);
  irc_socketId = socketId;
  irc_ssl = ssl;

  log(LOG_DEBUG, "Registering as '%s' and gecos '%s'", user, gecos);
  irc_write("USER %s %s %s :%s\r\n", user, user, user, gecos);
  log(LOG_DEBUG, "Using nickname '%s'", nick);
  irc_write("NICK %s\r\n", nick);

  return true;
}

bool irc_setNonBlocking(int socketId) {
  // Get the current flags set for the socket
  int flags = fcntl(socketId, F_GETFL, 0);
  if (flags == -1) {
    log(LOG_ERROR, "Unable to get current socket descriptor flags");
    return false;
  }

  // Set the socket to be non-blocking
  flags = fcntl(socketId, F_SETFL, flags | O_NONBLOCK);
  if (flags == -1) {
    log(LOG_ERROR, "Unable to set socket descriptor flags");
    return false;
  }

  return true;
}

const char *irc_readLine(SSL *ssl, int socketId, int timeout, size_t maxBytes) {
  size_t offset = 0;
  uint8_t timeouts = 0;
  while (true) {
    bool dataIsAvailable = irc_pollForData(ssl, socketId, timeout);
    if (timeouts++ > 5) {
      log(LOG_WARNING, "Peer timed out five times");
      // TODO: This should be handled much better, may drop messages right now
      ssize_t bytesAvailable = irc_getAvailableBytes(ssl);
      char *buffer = 0;
      irc_readSSLBytes(ssl, &buffer, bytesAvailable, READ_FLAGS_NONE);
      return irc_readLine(ssl, socketId, timeout, maxBytes);
    }
    if (!dataIsAvailable)
      continue;

    ssize_t bytesAvailable = irc_getAvailableBytes(ssl);
    if (bytesAvailable == 0) {
      // Nothing to read, wait for next event as specified by the polling above
      continue;
    } else if (bytesAvailable < 0) {
      // Failed to get available bytes
      return 0;
    } else if ((size_t)bytesAvailable >= maxBytes) {
      // Don't process more than max bytes
      bytesAvailable = maxBytes;
    }

    // Read the request without consuming the content
    char *buffer = 0;
    size_t bytesReceived = 0;
    bytesReceived = irc_readSSLBytes(ssl, &buffer, bytesAvailable, READ_FLAGS_PEEK);
    // Nothing read, wait for next event
    if (buffer == 0)
      continue;

    ssize_t lineLength = -1;
    for (; offset < bytesReceived; offset++) {
      if (buffer[offset] == '\n') {
        lineLength = offset + 1;
        break;
      }
    }

    if (lineLength < 1) {
      if ((size_t)bytesAvailable >= maxBytes) {
        log(LOG_WARNING, "No line found without looking for more than max bytes");
        return 0;
      }

      // There's no line available yet, wait for next event
      continue;
    }

    bytesReceived = irc_readSSLBytes(ssl, &buffer, lineLength, READ_FLAGS_NONE);
    // Could not read message
    if (buffer == 0)
      return 0;

    // Strip trailing CRLF
    buffer[bytesReceived - 1] = 0;
    buffer[bytesReceived - 2] = 0;

    return buffer;
  }
}

bool irc_pollForData(SSL *ssl, int socketId, int timeout) {
  if (ssl != 0 && SSL_pending(ssl) > 0)
    return true;

  // Set up structures necessary for polling
  struct pollfd descriptors[1];
  memset(descriptors, 0, sizeof(struct pollfd));
  descriptors[0].fd = socketId;
  descriptors[0].events = POLLIN;

  log(LOG_DEBUG, "Waiting for data to be readable");

  // Wait for the connection to be ready to read
  int status = poll(descriptors, 1, timeout);
  if (status < -1) {
    log(LOG_ERROR, "Could not wait for connection to send data");
    return false;
  } else if (status == 0) {
    log(LOG_ERROR, "The connection timed out");
    return false;
  }

  return true;
}

ssize_t irc_getAvailableBytes(SSL *ssl) {
  // Get the number of bytes immediately available for reading
  int bytesAvailable = -1;
  bytesAvailable = SSL_pending(ssl);
  if (bytesAvailable < 0) {
    log(LOG_ERROR, "Failed to get number of available bytes from TLS connection");
    return -1;
  } else if (bytesAvailable == 0) {
    // OpenSSL only returns bytes available for reading. Unless any read has been done,
    // there will be no bytes process for reading. Therefore we need to read at least once
    // before we can use SSL_pending()
    char *buffer = 0;
    // Read one byte without blocking to make OpenSSL process bytes
    irc_readSSLBytes(ssl, &buffer, 1, READ_FLAGS_PEEK);
    if (buffer != 0)
      free(buffer);
  }

  log(LOG_DEBUG, "There are %d bytes available for reading from TLS connection", bytesAvailable);

  return (ssize_t)bytesAvailable;
}

size_t irc_readSSLBytes(SSL *ssl, char **buffer, size_t bytesToRead, int flags) {
  (*buffer) = malloc(sizeof(char) * bytesToRead);
  size_t bytesReceived = 0;
  int result = 0;
  if (flags == READ_FLAGS_PEEK)
    result = SSL_peek_ex(ssl, *buffer, bytesToRead, &bytesReceived);
  else
    result = SSL_read_ex(ssl, *buffer, bytesToRead, &bytesReceived);

  if (result != 1) {
    int error = SSL_get_error(ssl, result);
    if (error == SSL_ERROR_WANT_READ)
      log(LOG_ERROR, "Could not write to peer. Socket wants read");
    else if (error == SSL_ERROR_WANT_WRITE)
      log(LOG_ERROR, "Could not write to peer. Socket wants write");
    else
      log(LOG_ERROR, "Could not write to peer. Got code %d (%s)", error, ERR_error_string(error, 0));

    return 0;
  }

  log(LOG_DEBUG, "Read or peeked %zu bytes", bytesReceived);

  return bytesReceived;
}

size_t irc_writeSSLBytes(SSL *ssl, const char *buffer, size_t bufferSize) {
  ssize_t bytesSent = 0;
  int result = SSL_write_ex(ssl, buffer, bufferSize, (size_t *)&bytesSent);
  if (result != 1) {
    int error = SSL_get_error(ssl, result);
    if (error == SSL_ERROR_WANT_READ)
      log(LOG_ERROR, "Could not write to peer. Socket wants read");
    else if (error == SSL_ERROR_WANT_WRITE)
      log(LOG_ERROR, "Could not write to peer. Socket wants write");
    else
      log(LOG_ERROR, "Could not write to peer. Got code %d (%s) - %s", error, ERR_error_string(error, 0), ERR_reason_error_string(error));

    return 0;
  }

  log(LOG_DEBUG, "Successfully wrote %zu (out of %zu) bytes to peer", bytesSent, bufferSize);
  return bytesSent;
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

  irc_writeSSLBytes(irc_ssl, message, messageLength);
  free(message);
}

const char *irc_read() {
  return irc_readLine(irc_ssl, irc_socketId, IRC_MESSAGE_TIMEOUT, IRC_MESSAGE_MAX_SIZE);
}

void irc_join(const char *channel) {
  irc_write("JOIN %s\r\n", channel);
}
