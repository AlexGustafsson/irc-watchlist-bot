#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <openssl/err.h>

#include "../logging/logging.h"

#include "tls.h"

static SSL_CTX *tls_sslContext = 0;

bool tls_initialize() {
  const SSL_METHOD *method = TLS_method();
  SSL_CTX *sslContext = SSL_CTX_new(method);
  if (sslContext == 0) {
    log(LOG_ERROR, "Unable to instantiate SSL context");
    return false;
  }

  // Only allow TLS 1.2 and above (TLS 1.3)
  SSL_CTX_set_min_proto_version(sslContext, TLS1_2_VERSION);
  SSL_CTX_set1_curves_list(sslContext, TLS_DEFAULT_ELLIPTIC_CURVES);
  // Set the cipher suite to use for TLS 1.2
  SSL_CTX_set_cipher_list(sslContext, TLS_DEFAULT_TLS_1_2_CIPHER_SUITE);
  // Set the cipher suite to use for TLS 1.3
  SSL_CTX_set_ciphersuites(sslContext, TLS_DEFAULT_TLS_1_3_CIPHER_SUITE);

  tls_sslContext = sslContext;
  return true;
}

tls_t *tls_connect(const char *hostname, uint16_t port) {
  tls_t *tls = malloc(sizeof(tls_t));
  if (tls == 0) {
    log(LOG_ERROR, "Unable to allocate TLS object");
    return 0;
  }
  memset(tls, 0, sizeof(tls_t));

  // Open a IPv4 TCP socket
  tls->socketId = socket(AF_INET, SOCK_STREAM, 0);
  if (tls->socketId == -1) {
    log(LOG_ERROR, "Unable to create a socket");
    return 0;
  }

  if (!tls_setNonBlocking(tls))
    return 0;

  struct hostent *hostent = gethostbyname(hostname);
  if (hostent == 0) {
    log(LOG_ERROR, "Unable to get host by name for server");
    return 0;
  }

  struct in_addr address = *(struct in_addr *)*(hostent->h_addr_list);

  struct sockaddr_in socketAddress;
  socketAddress.sin_addr.s_addr = address.s_addr;
  socketAddress.sin_family = AF_INET;
  socketAddress.sin_port = htons(port);

  if (connect(tls->socketId, (struct sockaddr *)&socketAddress, sizeof(struct sockaddr_in)) == 0) {
    log(LOG_ERROR, "Unable to connect to server: %s", strerror(errno));
    return 0;
  }

  tls->ssl = SSL_new(tls_sslContext);
  if (tls->ssl == 0) {
    log(LOG_ERROR, "Unable to instantiate SSL object");
    return 0;
  }

  SSL_set_fd(tls->ssl, tls->socketId);
  if (SSL_set_tlsext_host_name(tls->ssl, hostname) != 1) {
    log(LOG_ERROR, "Unable to set the server's hostname");
    SSL_free(tls->ssl);
    return 0;
  }

  // Set up structures necessary for polling
  struct pollfd readDescriptors[1];
  memset(readDescriptors, 0, sizeof(struct pollfd));
  readDescriptors[0].fd = tls->socketId;
  readDescriptors[0].events = POLLIN;

  struct pollfd writeDescriptors[1];
  memset(writeDescriptors, 0, sizeof(struct pollfd));
  writeDescriptors[0].fd = tls->socketId;
  writeDescriptors[0].events = POLLOUT;

  while (true) {
    int status = SSL_connect(tls->ssl);
    if (status == 0) {
      log(LOG_ERROR, "Unable to connect to server");
      SSL_free(tls->ssl);

      return 0;
    } else if (status < 0) {
      int error = SSL_get_error(tls->ssl, status);
      if (error == SSL_ERROR_WANT_READ) {
        log(LOG_DEBUG, "Waiting for TLS connection to be readable");
        // Wait for the connection to be ready to read
        int status = poll(readDescriptors, 1, -1);
        if (status < -1) {
          log(LOG_ERROR, "Could not wait for connection to be readable");
          SSL_free(tls->ssl);

          return 0;
        }
      } else if (error == SSL_ERROR_WANT_WRITE) {
        log(LOG_DEBUG, "Waiting for TLS connection to be writable");
        // Wait for the connection to be ready to write
        int status = poll(writeDescriptors, 1, -1);
        if (status < -1) {
          log(LOG_ERROR, "Could not wait for connection to be writable");
          SSL_free(tls->ssl);

          return 0;
        }
      } else {
        log(LOG_ERROR, "Unable to connect to server. Got code %d", error);
        SSL_free(tls->ssl);

        return 0;
      }
    } else {
      break;
    }
  }

  return tls;
}

bool tls_setNonBlocking(tls_t *tls) {
  // Get the current flags set for the socket
  int flags = fcntl(tls->socketId, F_GETFL, 0);
  if (flags == -1) {
    log(LOG_ERROR, "Unable to get current socket descriptor flags");
    return false;
  }

  // Set the socket to be non-blocking
  flags = fcntl(tls->socketId, F_SETFL, flags | O_NONBLOCK);
  if (flags == -1) {
    log(LOG_ERROR, "Unable to set socket descriptor flags");
    return false;
  }

  return true;
}

char *tls_readLine(tls_t *tls, int timeout, size_t maxBytes) {
  size_t offset = 0;
  while (true) {
    int pollStatus = tls_pollForData(tls, timeout);

    if (pollStatus == TLS_POLL_STATUS_FAILED) {
      log(LOG_ERROR, "Unable to poll");
      return 0;
    }

    if (pollStatus == TLS_POLL_STATUS_NOT_AVAILABLE) {
      log(LOG_DEBUG, "Polling timed out");
      return 0;
    }

    ssize_t bytesAvailable = tls_getAvailableBytes(tls);
    if (bytesAvailable == 0) {
      // Nothing to read, wait for next event as specified by the polling above
      continue;
    } else if (bytesAvailable < 0) {
      log(LOG_ERROR, "Unable to get the number of available bytes");
      return 0;
    } else if ((size_t)bytesAvailable >= maxBytes) {
      // Don't process more than max bytes
      bytesAvailable = maxBytes;
    }

    // Read the request without consuming the content
    char *buffer = 0;
    size_t bytesReceived = 0;
    bytesReceived = tls_read(tls, &buffer, bytesAvailable, READ_FLAGS_NONE);
    // Nothing read, wait for next event
    if (buffer == 0)
      continue;

    // Add the read buffer to the connection's buffer
    if (tls->buffer == 0) {
      tls->buffer = buffer;
    } else {
      tls->buffer = realloc(tls->buffer, sizeof(char) * (tls->bufferSize + bytesReceived));
      memcpy(tls->buffer + tls->bufferSize, buffer, bytesReceived);
      tls->bufferSize += bytesReceived;
    }

    ssize_t lineLength = -1;
    for (; offset < bytesReceived; offset++) {
      if (tls->buffer[offset] == '\n') {
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

    // Strip trailing CRLF
    tls->buffer[bytesReceived - 1] = 0;
    tls->buffer[bytesReceived - 2] = 0;

    char *result = tls->buffer;
    tls->buffer = 0;
    tls->bufferSize = 0;

    return result;
  }
}

int tls_pollForData(tls_t *tls, int timeout) {
  if (SSL_pending(tls->ssl) > 0)
    return TLS_POLL_STATUS_AVAILABLE;

  // Set up structures necessary for polling
  struct pollfd descriptors[1];
  memset(descriptors, 0, sizeof(struct pollfd));
  descriptors[0].fd = tls->socketId;
  descriptors[0].events = POLLIN;

  log(LOG_DEBUG, "Waiting for data to be readable");

  // Wait for the connection to be ready to read
  int status = poll(descriptors, 1, timeout);
  if (status < -1) {
    log(LOG_ERROR, "Could not wait for connection to send data");
    return TLS_POLL_STATUS_FAILED;
  } else if (status == 0) {
    log(LOG_ERROR, "The connection timed out");
    return TLS_POLL_STATUS_NOT_AVAILABLE;
  }

  return TLS_POLL_STATUS_AVAILABLE;
}

ssize_t tls_getAvailableBytes(tls_t *tls) {
  // Get the number of bytes immediately available for reading
  int bytesAvailable = -1;
  bytesAvailable = SSL_pending(tls->ssl);
  if (bytesAvailable < 0) {
    log(LOG_ERROR, "Failed to get number of available bytes from TLS connection");
    return -1;
  } else if (bytesAvailable == 0) {
    // OpenSSL only returns bytes available for reading. Unless any read has been done,
    // there will be no bytes process for reading. Therefore we need to read at least once
    // before we can use SSL_pending()
    char *buffer = 0;
    // Read one byte without blocking to make OpenSSL process bytes
    tls_read(tls, &buffer, 1, READ_FLAGS_PEEK);
    if (buffer != 0)
      free(buffer);
  }

  log(LOG_DEBUG, "There are %d bytes available for reading from TLS connection", bytesAvailable);

  return (ssize_t)bytesAvailable;
}

size_t tls_read(tls_t *tls, char **buffer, size_t bytesToRead, int flags) {
  (*buffer) = malloc(sizeof(char) * bytesToRead);
  size_t bytesReceived = 0;
  int result = 0;
  if (flags == READ_FLAGS_PEEK)
    result = SSL_peek_ex(tls->ssl, *buffer, bytesToRead, &bytesReceived);
  else
    result = SSL_read_ex(tls->ssl, *buffer, bytesToRead, &bytesReceived);

  if (result != 1) {
    int error = SSL_get_error(tls->ssl, result);
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

size_t tls_write(tls_t *tls, const char *buffer, size_t bufferSize) {
  ssize_t bytesSent = 0;
  int result = SSL_write_ex(tls->ssl, buffer, bufferSize, (size_t *)&bytesSent);
  if (result != 1) {
    int error = SSL_get_error(tls->ssl, result);
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

void tls_disconnect(tls_t *tls) {
  if (shutdown(tls->socketId, SHUT_RDWR) == -1) {
    if (errno != ENOTCONN && errno != EINVAL) {
      if (errno == ENOTSOCK || errno == EBADF) {
        log(LOG_ERROR, "Failed to shutdown TLS connection. It was likely already closed");
      } else {
        const char *reason = strerror(errno);
        log(LOG_ERROR, "Failed to shutdown TLS connection. Got error %d (%s)", errno, reason);
      }
    }
  } else {
    if (close(tls->socketId) == -1)
      log(LOG_ERROR, "Unable to close TLS connection. Got error %d (%s)", errno, strerror(errno));
  }
  SSL_free(tls->ssl);
  free(tls);
}

void tls_free(tls_t *tls) {
  SSL_free(tls->ssl);
  if (tls->buffer != 0)
    free(tls->buffer);
}
