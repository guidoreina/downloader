#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <sys/uio.h>
#include "net/socket_address.h"

namespace net {
  class socket {
    public:
      static const unsigned kBacklog = 128;

      enum class type {
        kStream = SOCK_STREAM,
        kDatagram = SOCK_DGRAM,
        kRaw = SOCK_RAW
      };

      enum class shutdown_type {
        kShutdownRecv = SHUT_RD,
        kShutdownSend = SHUT_WR,
        kShutdownRecvSend = SHUT_RDWR
      };

      // Constructor.
      socket();
      socket(int fd);

      // Create socket.
      bool create(int domain, type type);

      // Close socket.
      bool close();

      // Shutdown socket.
      bool shutdown(shutdown_type how);

      // Get socket error.
      bool get_socket_error(int& error) const;

      // Connect.
      bool connect(type type, const socket_address& addr, int timeout = -1);

      // Bind.
      bool bind(const socket_address& addr);

      // Listen.
      bool listen(const socket_address& addr);

      // Accept.
      bool accept(socket& s, int timeout = -1);
      bool accept(socket& s, socket_address& addr, int timeout = -1);

      // Read.
      ssize_t read(void* buf, size_t count, int timeout = -1);

      // Read into multiple buffers.
      ssize_t readv(const struct iovec* iov,
                    unsigned iovcnt,
                    int timeout = -1);

      // Receive UDP datagram.
      ssize_t recvfrom(void* buf, size_t len, int timeout = -1);
      ssize_t recvfrom(void* buf,
                       size_t len,
                       socket_address& addr,
                       int timeout = -1);

      // Write.
      ssize_t write(const void* buf, size_t count, int timeout = -1);

      // Write from multiple buffers.
      ssize_t writev(const struct iovec* iov,
                     unsigned iovcnt,
                     int timeout = -1);

      // Send UDP datagram.
      bool sendto(const void* buf,
                  size_t len,
                  const socket_address& addr,
                  int timeout = -1);

      // Get receive buffer size.
      bool get_recvbuf_size(int& size) const;

      // Set receive buffer size.
      bool set_recvbuf_size(int size);

      // Get send buffer size.
      bool get_sendbuf_size(int& size) const;

      // Set send buffer size.
      bool set_sendbuf_size(int size);

      // Get TCP no delay.
      bool get_tcp_no_delay(bool& on) const;

      // Set TCP no delay.
      bool set_tcp_no_delay(bool on);

      // Cork.
      bool cork();

      // Uncork.
      bool uncork();

      // Wait readable.
      bool wait_readable(int timeout);

      // Wait writable.
      bool wait_writable(int timeout);

      // Get socket descriptor.
      int fd() const;

      // Set socket descriptor.
      void fd(int descriptor);

    private:
      int _M_fd;

      // Make socket non-blocking.
      bool _M_set_non_blocking();

      // Accept.
      bool _M_accept(socket& s,
                     socket_address* addr,
                     socklen_t* addrlen,
                     int timeout);

      // Receive UDP datagram.
      ssize_t _M_recvfrom(void* buf,
                          size_t len,
                          socket_address* addr,
                          socklen_t* addrlen,
                          int timeout);

      // Disable copy constructor and assignment operator.
      socket(const socket&) = delete;
      socket& operator=(const socket&) = delete;
  };

  inline socket::socket()
  {
  }

  inline socket::socket(int fd)
    : _M_fd(fd)
  {
  }

  inline bool socket::accept(socket& s, int timeout)
  {
    return _M_accept(s, NULL, NULL, timeout);
  }

  inline bool socket::accept(socket& s, socket_address& addr, int timeout)
  {
    socklen_t addrlen = sizeof(socket_address);
    return _M_accept(s, &addr, &addrlen, timeout);
  }

  inline ssize_t socket::recvfrom(void* buf, size_t len, int timeout)
  {
    return _M_recvfrom(buf, len, NULL, NULL, timeout);
  }

  inline ssize_t socket::recvfrom(void* buf,
                                  size_t len,
                                  socket_address& addr,
                                  int timeout)
  {
    socklen_t addrlen = sizeof(socket_address);
    return _M_recvfrom(buf, len, &addr, &addrlen, timeout);
  }

  inline int socket::fd() const
  {
    return _M_fd;
  }

  inline void socket::fd(int descriptor)
  {
    _M_fd = descriptor;
  }
}

#endif // NET_SOCKET_H
