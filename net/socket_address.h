#ifndef NET_SOCKET_ADDRESS_H
#define NET_SOCKET_ADDRESS_H

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace net {
  class socket_address : public sockaddr_storage {
    public:
      // Constructor.
      socket_address();
      socket_address(const socket_address& addr);

      // Get size.
      size_t size() const;

      // Build.
      bool build(const char* address, in_port_t port);
      bool build(const char* address);

      // Comparison operator.
      bool operator==(const socket_address& addr) const;

      // Assignment operator.
      socket_address& operator=(const socket_address& addr);
  };

  inline socket_address::socket_address()
  {
  }

  inline socket_address::socket_address(const socket_address& addr)
  {
    memcpy(this, &addr, addr.size());
  }

  inline socket_address& socket_address::operator=(const socket_address& addr)
  {
    memcpy(this, &addr, addr.size());
    return *this;
  }
}

#endif // NET_SOCKET_ADDRESS_H
