#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if HAVE_POLL
  #include <poll.h>
#else
  #include <sys/select.h>
#endif

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <limits.h>
#include <errno.h>
#include "net/socket.h"
#include "net/local_address.h"

bool net::socket::create(int domain, type type)
{
  if ((_M_fd = ::socket(domain, static_cast<int>(type), 0)) < 0) {
    return false;
  }

  if (!_M_set_non_blocking()) {
    close();
    return false;
  }

  return true;
}

bool net::socket::close()
{
  if (::close(_M_fd) < 0) {
    return false;
  }

  return true;
}

bool net::socket::shutdown(shutdown_type how)
{
  if (::shutdown(_M_fd, static_cast<int>(how)) < 0) {
    return false;
  }

  return true;
}

bool net::socket::get_socket_error(int& error) const
{
  socklen_t errorlen = sizeof(int);
  if (getsockopt(_M_fd, SOL_SOCKET, SO_ERROR, &error, &errorlen) < 0) {
    return false;
  }

  return true;
}

bool net::socket::connect(type type, const socket_address& addr, int timeout)
{
  // Create socket.
  if (!create(addr.ss_family, type)) {
    return false;
  }

  // Connect.
  if ((::connect(_M_fd,
                 reinterpret_cast<const struct sockaddr*>(&addr),
                 addr.size()) < 0) &&
      (errno != EINPROGRESS)) {
    close();
    return false;
  }

  if (timeout != 0) {
    if (!wait_writable(timeout)) {
      close();
      return false;
    }

    int error;
    if ((!get_socket_error(error)) || (error != 0)) {
      close();
      return false;
    }
  }

  return true;
}

bool net::socket::bind(const socket_address& addr)
{
  // Reuse address.
  int optval = 1;
  if (setsockopt(_M_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
    return false;
  }

  if (addr.ss_family == AF_UNIX) {
    unlink(reinterpret_cast<const local_address*>(&addr)->sun_path);
  }

  // Bind.
  if (::bind(_M_fd,
             reinterpret_cast<const struct sockaddr*>(&addr),
             addr.size()) < 0) {
    return false;
  }

  return true;
}

bool net::socket::listen(const socket_address& addr)
{
  // Create socket.
  if (!create(addr.ss_family, type::kStream)) {
    return false;
  }

  // Bind.
  if (!bind(addr)) {
    close();
    return false;
  }

  // Listen.
  if (::listen(_M_fd, kBacklog) < 0) {
    if (addr.ss_family == AF_UNIX) {
      unlink(reinterpret_cast<const local_address*>(&addr)->sun_path);
    }

    close();
    return false;
  }

  return true;
}

ssize_t net::socket::read(void* buf, size_t count, int timeout)
{
  ssize_t ret;

  if (timeout == 0) {
    while (((ret = recv(_M_fd, buf, count, 0)) < 0) && (errno == EINTR));
    return ret;
  } else {
    do {
      switch (ret = recv(_M_fd, buf, count, 0)) {
        case -1:
          switch (errno) {
            case EAGAIN:
              if (!wait_readable(timeout)) {
                return -1;
              }

              break;
            case EINTR:
              break;
            default:
              return -1;
          }

          break;
        case 0:
          return 0;
        default:
          return ret;
      }
    } while (true);
  }
}

ssize_t net::socket::readv(const struct iovec* iov,
                           unsigned iovcnt,
                           int timeout)
{
  ssize_t ret;

  if (timeout == 0) {
    while (((ret = ::readv(_M_fd, iov, iovcnt)) < 0) && (errno == EINTR));
    return ret;
  } else {
    do {
      switch (ret = ::readv(_M_fd, iov, iovcnt)) {
        case -1:
          switch (errno) {
            case EAGAIN:
              if (!wait_readable(timeout)) {
                return -1;
              }

              break;
            case EINTR:
              break;
            default:
              return -1;
          }

          break;
        case 0:
          return 0;
        default:
          return ret;
      }
    } while (true);
  }
}

ssize_t net::socket::write(const void* buf, size_t count, int timeout)
{
  if (timeout == 0) {
    ssize_t ret;
    while (((ret = send(_M_fd, buf, count, 0)) < 0) && (errno == EINTR));

    return ret;
  } else {
    const uint8_t* b = reinterpret_cast<const uint8_t*>(buf);
    size_t written = 0;

    do {
      ssize_t ret;
      if ((ret = send(_M_fd, b, count - written, 0)) < 0) {
        switch (errno) {
          case EAGAIN:
            if (!wait_writable(timeout)) {
              return -1;
            }

            break;
          case EINTR:
            break;
          default:
            return -1;
        }
      } else if (ret > 0) {
        if ((written += ret) == count) {
          return count;
        }

        if (!wait_writable(timeout)) {
          return -1;
        }

        b += ret;
      }
    } while (true);
  }
}

ssize_t net::socket::writev(const struct iovec* iov,
                            unsigned iovcnt,
                            int timeout)
{
  if (timeout == 0) {
    ssize_t ret;
    while (((ret = ::writev(_M_fd, iov, iovcnt)) < 0) && (errno == EINTR));

    return ret;
  } else {
    if (iovcnt > IOV_MAX) {
      errno = EINVAL;
      return -1;
    }

    struct iovec vec[IOV_MAX];
    size_t total = 0;

    for (unsigned i = 0; i < iovcnt; i++) {
      vec[i].iov_base = iov[i].iov_base;
      vec[i].iov_len = iov[i].iov_len;

      total += vec[i].iov_len;
    }

    struct iovec* v = vec;
    size_t written = 0;

    do {
      ssize_t ret;
      if ((ret = ::writev(_M_fd, v, iovcnt)) < 0) {
        switch (errno) {
          case EAGAIN:
            if (!wait_writable(timeout)) {
              return -1;
            }

            break;
          case EINTR:
            break;
          default:
            return -1;
        }
      } else if (ret > 0) {
        if ((written += ret) == total) {
          return total;
        }

        if (!wait_writable(timeout)) {
          return -1;
        }

        while (static_cast<size_t>(ret) >= v->iov_len) {
          ret -= v->iov_len;

          v++;
          iovcnt--;
        }

        if (ret > 0) {
          v->iov_base = reinterpret_cast<uint8_t*>(v->iov_base) + ret;
          v->iov_len -= ret;
        }
      }
    } while (true);
  }
}

bool net::socket::sendto(const void* buf,
                         size_t len,
                         const socket_address& addr,
                         int timeout)
{
  ssize_t ret;

  if (timeout == 0) {
    while (((ret = ::sendto(_M_fd,
                            buf,
                            len,
                            0,
                            reinterpret_cast<const struct sockaddr*>(&addr),
                            addr.size())) < 0) &&
           (errno == EINTR));
  } else {
    do {
      if ((ret = ::sendto(_M_fd,
                          buf,
                          len,
                          0,
                          reinterpret_cast<const struct sockaddr*>(&addr),
                          addr.size())) < 0) {
        switch (errno) {
          case EAGAIN:
            if (!wait_writable(timeout)) {
              return false;
            }

            break;
          case EINTR:
            break;
          default:
            return false;
        }
      } else {
        break;
      }
    } while (true);
  }

  return (ret == static_cast<ssize_t>(len));
}

bool net::socket::get_recvbuf_size(int& size) const
{
  socklen_t optlen = sizeof(int);
  if (getsockopt(_M_fd, SOL_SOCKET, SO_RCVBUF, &size, &optlen) < 0) {
    return false;
  }

  return true;
}

bool net::socket::set_recvbuf_size(int size)
{
  if (setsockopt(_M_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int)) < 0) {
    return false;
  }

  return true;
}

bool net::socket::get_sendbuf_size(int& size) const
{
  socklen_t optlen = sizeof(int);
  if (getsockopt(_M_fd, SOL_SOCKET, SO_SNDBUF, &size, &optlen) < 0) {
    return false;
  }

  return true;
}

bool net::socket::set_sendbuf_size(int size)
{
  if (setsockopt(_M_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int)) < 0) {
    return false;
  }

  return true;
}

bool net::socket::get_tcp_no_delay(bool& on) const
{
#ifndef __minix
  int optval;
  socklen_t optlen = sizeof(int);
  if (getsockopt(_M_fd, IPPROTO_TCP, TCP_NODELAY, &optval, &optlen) < 0) {
    return false;
  }

  on = (optval != 0);

  return true;
#else
  return false;
#endif
}

bool net::socket::set_tcp_no_delay(bool on)
{
#ifndef __minix
  int optval = (on == true);
  if (setsockopt(_M_fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int)) < 0) {
    return false;
  }
#endif // !__minix

  return true;
}

bool net::socket::cork()
{
#if HAVE_TCP_CORK
  int optval = 1;
  if (setsockopt(_M_fd, IPPROTO_TCP, TCP_CORK, &optval, sizeof(int)) < 0) {
    return false;
  }
#elif HAVE_TCP_NOPUSH
  int optval = 1;
  if (setsockopt(_M_fd, IPPROTO_TCP, TCP_NOPUSH, &optval, sizeof(int)) < 0) {
    return false;
  }
#endif

  return true;
}

bool net::socket::uncork()
{
#if HAVE_TCP_CORK
  int optval = 0;
  if (setsockopt(_M_fd, IPPROTO_TCP, TCP_CORK, &optval, sizeof(int)) < 0) {
    return false;
  }
#elif HAVE_TCP_NOPUSH
  int optval = 0;
  if (setsockopt(_M_fd, IPPROTO_TCP, TCP_NOPUSH, &optval, sizeof(int)) < 0) {
    return false;
  }
#endif

  return true;
}

bool net::socket::wait_readable(int timeout)
{
#if HAVE_POLL
  struct pollfd fd;
  fd.fd = _M_fd;

#if HAVE_POLLRDHUP
  fd.events = POLLIN | POLLRDHUP;
#else
  fd.events = POLLIN;
#endif

  fd.revents = 0;

  return (poll(&fd, 1, timeout) > 0);
#else
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(_M_fd, &rfds);

  if (timeout < 0) {
    return (select(_M_fd + 1, &rfds, NULL, NULL, NULL) > 0);
  } else {
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    return (select(_M_fd + 1, &rfds, NULL, NULL, &tv) > 0);
  }
#endif
}

bool net::socket::wait_writable(int timeout)
{
#if HAVE_POLL
  struct pollfd fd;
  fd.fd = _M_fd;
  fd.events = POLLOUT;
  fd.revents = 0;

  return (poll(&fd, 1, timeout) > 0);
#else
  fd_set wfds;
  FD_ZERO(&wfds);
  FD_SET(_M_fd, &wfds);

  if (timeout < 0) {
    return (select(_M_fd + 1, NULL, &wfds, NULL, NULL) > 0);
  } else {
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    return (select(_M_fd + 1, NULL, &wfds, NULL, &tv) > 0);
  }
#endif
}

bool net::socket::_M_set_non_blocking()
{
#if USE_FIONBIO
  int value = 1;
  if (ioctl(_M_fd, FIONBIO, &value) < 0) {
    return false;
  }
#else
  int flags = fcntl(_M_fd, F_GETFL);
  flags |= O_NONBLOCK;

  if (fcntl(_M_fd, F_SETFL, flags) < 0) {
    return false;
  }
#endif

  return true;
}

bool net::socket::_M_accept(socket& s,
                            socket_address* addr,
                            socklen_t* addrlen,
                            int timeout)
{
  if (timeout != 0) {
    if (!wait_readable(timeout)) {
      return false;
    }
  }

  int fd;

#if HAVE_ACCEPT4
  if ((fd = accept4(_M_fd,
                    reinterpret_cast<struct sockaddr*>(addr),
                    addrlen,
                    SOCK_NONBLOCK)) < 0) {
    return false;
  }

  s._M_fd = fd;
#else
  if ((fd = ::accept(_M_fd,
                     reinterpret_cast<struct sockaddr*>(addr),
                     addrlen)) < 0) {
    return false;
  }

  s._M_fd = fd;

  if (!s._M_set_non_blocking()) {
    s.close();
    return false;
  }
#endif

  return true;
}

ssize_t net::socket::_M_recvfrom(void* buf,
                                 size_t len,
                                 socket_address* addr,
                                 socklen_t* addrlen,
                                 int timeout)
{
  ssize_t ret;

  if (timeout == 0) {
    while (((ret = ::recvfrom(_M_fd,
                              buf,
                              len,
                              0,
                              reinterpret_cast<struct sockaddr*>(&addr),
                              addrlen)) < 0) &&
           (errno == EINTR));
  } else {
    do {
      if ((ret = ::recvfrom(_M_fd,
                            buf,
                            len,
                            0,
                            reinterpret_cast<struct sockaddr*>(&addr),
                            addrlen)) < 0) {
        switch (errno) {
          case EAGAIN:
            if (!wait_readable(timeout)) {
              return -1;
            }

            break;
          case EINTR:
            break;
          default:
            return -1;
        }
      } else {
        break;
      }
    } while (true);
  }

  return ret;
}
