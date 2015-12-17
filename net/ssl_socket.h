#ifndef NET_SSL_SOCKET_H
#define NET_SSL_SOCKET_H

#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include "net/socket.h"

namespace net {
  class ssl_socket {
    public:
      // Initialize SSL library.
      static bool init_ssl_library();

      // Free SSL library.
      static void free_ssl_library();

      // Load certificate.
      static bool load_certificate(const char* certificate, const char* key);

      // Constructor.
      ssl_socket();
      ssl_socket(int fd);
      ssl_socket(const socket& s);

      // Close socket.
      bool close();

      // Free.
      void free();

      // Perform TLS/SSL handshake.
      enum class ssl_mode {
        kClientMode,
        kServerMode
      };

      bool handshake(ssl_mode mode, bool& readable, bool& writable);
      bool handshake(ssl_mode mode, int timeout = -1);

      // Shutdown TLS/SSL connection.
      bool shutdown(bool bidirectional, bool& readable, bool& writable);
      bool shutdown(bool bidirectional, int timeout = -1);

      // Read.
      ssize_t read(void* buf, size_t count, bool& readable, bool& writable);
      ssize_t read(void* buf, size_t count, int timeout = -1);

      // Write.
      ssize_t write(const void* buf,
                    size_t count,
                    bool& readable,
                    bool& writable);

      ssize_t write(const void* buf, size_t count, int timeout = -1);

      // Get socket descriptor.
      int fd() const;

      // Set socket descriptor.
      void fd(int descriptor);

    private:
      static SSL_CTX* _M_ctx;

      socket _M_socket;
      SSL* _M_ssl;

      // Perform TLS/SSL handshake.
      bool _M_ssl_handshake(bool& readable, bool& writable);

      // Initialize SSL structure.
      bool _M_init_ssl_struct(ssl_mode mode);

      // Disable copy constructor and assignment operator.
      ssl_socket(const ssl_socket&) = delete;
      ssl_socket& operator=(const ssl_socket&) = delete;
  };

  inline ssl_socket::ssl_socket()
    : _M_ssl(NULL)
  {
  }

  inline ssl_socket::ssl_socket(int fd)
    : _M_socket(fd),
      _M_ssl(NULL)
  {
  }

  inline ssl_socket::ssl_socket(const socket& s)
    : _M_socket(s.fd()),
      _M_ssl(NULL)
  {
  }

  inline bool ssl_socket::close()
  {
    free();
    return _M_socket.close();
  }

  inline void ssl_socket::free()
  {
    if (_M_ssl) {
      SSL_free(_M_ssl);
      _M_ssl = NULL;
    }
  }

  inline int ssl_socket::fd() const
  {
    return _M_socket.fd();
  }

  inline void ssl_socket::fd(int descriptor)
  {
    _M_socket.fd(descriptor);
  }
}

#endif // NET_SSL_SOCKET_H
