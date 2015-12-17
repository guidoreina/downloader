#include <openssl/engine.h>
#include <openssl/objects.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <errno.h>
#include "net/ssl_socket.h"

SSL_CTX* net::ssl_socket::_M_ctx = NULL;

bool net::ssl_socket::init_ssl_library()
{
  SSL_load_error_strings();
  SSL_library_init();

  // Create SSL context.
  if ((_M_ctx = SSL_CTX_new(SSLv23_method())) == NULL) {
    free_ssl_library();
    return false;
  }

  return true;
}

void net::ssl_socket::free_ssl_library()
{
  if (_M_ctx) {
    SSL_CTX_free(_M_ctx);
    _M_ctx = NULL;
  }

  CONF_modules_unload(1);
  OBJ_cleanup();
  EVP_cleanup();
  ENGINE_cleanup();
  CRYPTO_cleanup_all_ex_data();

#if OPENSSL_VERSION_NUMBER >= 0x1000103fL
  ERR_remove_thread_state(NULL);
#else
  ERR_remove_state(0);
#endif

  ERR_free_strings();
}

bool net::ssl_socket::load_certificate(const char* certificate, const char* key)
{
  if (SSL_CTX_use_certificate_chain_file(_M_ctx, certificate) != 1) {
    return false;
  }

  if (SSL_CTX_use_PrivateKey_file(_M_ctx, key, SSL_FILETYPE_PEM) != 1) {
    return false;
  }

  return true;
}

bool net::ssl_socket::handshake(ssl_mode mode, bool& readable, bool& writable)
{
  if (!_M_ssl) {
    if (!_M_init_ssl_struct(mode)) {
      return false;
    }
  }

  return _M_ssl_handshake(readable, writable);
}

bool net::ssl_socket::handshake(ssl_mode mode, int timeout)
{
  if (!_M_init_ssl_struct(mode)) {
    return false;
  }

  do {
    bool readable = true, writable = true;
    if (!_M_ssl_handshake(readable, writable)) {
      if (!readable) {
        if (!_M_socket.wait_readable(timeout)) {
          return false;
        }
      } else if (!writable) {
        if (!_M_socket.wait_writable(timeout)) {
          return false;
        }
      } else {
        return false;
      }
    } else {
      return true;
    }
  } while (true);
}

bool net::ssl_socket::shutdown(bool bidirectional,
                               bool& readable,
                               bool& writable)
{
  if (!bidirectional) {
    SSL_set_shutdown(_M_ssl, SSL_get_shutdown(_M_ssl) | SSL_RECEIVED_SHUTDOWN);
  }

  do {
    // Clear the error queue.
    ERR_clear_error();

    int ret;
    int err;
    switch (ret = SSL_shutdown(_M_ssl)) {
      case 1:
        // The shutdown was successfully completed.

        SSL_free(_M_ssl);
        _M_ssl = NULL;

        return true;
      case 0:
        // The shutdown is not yet finished. Call SSL_shutdown()
        // for a second time, if a bidirectional shutdown shall be performed.
        break;
      default:
        switch (err = SSL_get_error(_M_ssl, ret)) {
          case SSL_ERROR_WANT_READ:
            readable = false;
            return false;
          case SSL_ERROR_WANT_WRITE:
            writable = false;
            return false;
          case SSL_ERROR_SYSCALL:
            if (errno == EINTR) {
              continue;
            }

            // Fall through.
          default:
            SSL_free(_M_ssl);
            _M_ssl = NULL;

            return false;
        }
    }
  } while (true);
}

bool net::ssl_socket::shutdown(bool bidirectional, int timeout)
{
  do {
    bool readable = true, writable = true;
    if (!shutdown(bidirectional, readable, writable)) {
      if (!readable) {
        if (!_M_socket.wait_readable(timeout)) {
          return false;
        }
      } else if (!writable) {
        if (!_M_socket.wait_writable(timeout)) {
          return false;
        }
      } else {
        return false;
      }
    } else {
      return true;
    }
  } while (true);
}

ssize_t net::ssl_socket::read(void* buf,
                              size_t count,
                              bool& readable,
                              bool& writable)
{
  do {
    // Clear the error queue.
    ERR_clear_error();

    int ret;
    if ((ret = SSL_read(_M_ssl, buf, count)) > 0) {
      return ret;
    }

    int err;
    switch (err = SSL_get_error(_M_ssl, ret)) {
      case SSL_ERROR_WANT_READ:
        readable = false;
        return -1;
      case SSL_ERROR_WANT_WRITE:
        writable = false;
        return -1;
      case SSL_ERROR_SYSCALL:
        if ((ret < 0) && (errno == EINTR)) {
          continue;
        }

        // Fall through.
      case SSL_ERROR_ZERO_RETURN:
        // The TLS/SSL connection has been closed.
      default:
        return -1;
    }
  } while (true);
}

ssize_t net::ssl_socket::read(void* buf, size_t count, int timeout)
{
  do {
    bool readable = true, writable = true;
    ssize_t ret;
    if ((ret = read(buf, count, readable, writable)) < 0) {
      if (!readable) {
        if (!_M_socket.wait_readable(timeout)) {
          return -1;
        }
      } else if (!writable) {
        if (!_M_socket.wait_writable(timeout)) {
          return -1;
        }
      } else {
        return -1;
      }
    } else {
      return ret;
    }
  } while (true);
}

ssize_t net::ssl_socket::write(const void* buf,
                               size_t count,
                               bool& readable,
                               bool& writable)
{
  do {
    // Clear the error queue.
    ERR_clear_error();

    int ret;
    if ((ret = SSL_write(_M_ssl, buf, count)) > 0) {
      return ret;
    }

    int err;
    switch (err = SSL_get_error(_M_ssl, ret)) {
      case SSL_ERROR_WANT_READ:
        readable = false;
        return -1;
      case SSL_ERROR_WANT_WRITE:
        writable = false;
        return -1;
      case SSL_ERROR_SYSCALL:
        if ((ret < 0) && (errno == EINTR)) {
          continue;
        }

        // Fall through.
      case SSL_ERROR_ZERO_RETURN:
        // The TLS/SSL connection has been closed.
      default:
        return -1;
    }
  } while (true);
}

ssize_t net::ssl_socket::write(const void* buf, size_t count, int timeout)
{
  do {
    bool readable = true, writable = true;
    ssize_t ret;
    if ((ret = write(buf, count, readable, writable)) < 0) {
      if (!readable) {
        if (!_M_socket.wait_readable(timeout)) {
          return -1;
        }
      } else if (!writable) {
        if (!_M_socket.wait_writable(timeout)) {
          return -1;
        }
      } else {
        return -1;
      }
    } else {
      return ret;
    }
  } while (true);
}

bool net::ssl_socket::_M_ssl_handshake(bool& readable, bool& writable)
{
  do {
    // Clear the error queue.
    ERR_clear_error();

    int ret;
    if ((ret = SSL_do_handshake(_M_ssl)) == 1) {
      return true;
    }

    int err;
    switch (err = SSL_get_error(_M_ssl, ret)) {
      case SSL_ERROR_WANT_READ:
        readable = false;
        return false;
      case SSL_ERROR_WANT_WRITE:
        writable = false;
        return false;
      case SSL_ERROR_SYSCALL:
        if ((ret < 0) && (errno == EINTR)) {
          continue;
        }

        // Fall through.
      default:
        return false;
    }
  } while (true);
}

bool net::ssl_socket::_M_init_ssl_struct(ssl_mode mode)
{
  if ((_M_ssl = SSL_new(_M_ctx)) == NULL) {
    return false;
  }

  if (!SSL_set_fd(_M_ssl, _M_socket.fd())) {
    SSL_free(_M_ssl);
    _M_ssl = NULL;

    return false;
  }

  if (mode == ssl_mode::kClientMode) {
    SSL_set_connect_state(_M_ssl);
  } else {
    SSL_set_accept_state(_M_ssl);
  }

  return true;
}
