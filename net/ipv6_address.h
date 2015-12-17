#ifndef NET_IPV6_ADDRESS_H
#define NET_IPV6_ADDRESS_H

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net/socket_address.h"

namespace net {
  class ipv6_address : public sockaddr_in6 {
    public:
      static const char* kAnyAddress;
      static const char* kLocalhost;

      // Constructor.
      ipv6_address();

      // Get port.
      in_port_t port() const;

      // Set port.
      void port(in_port_t p);

      // Cast operators.
      operator const socket_address*() const;
      operator socket_address*();

      operator const socket_address&() const;
      operator socket_address&();
  };

  inline ipv6_address::ipv6_address()
  {
    memset(this, 0, sizeof(ipv6_address));

    sin6_family = AF_INET6;
  }

  inline in_port_t ipv6_address::port() const
  {
    return ntohs(sin6_port);
  }

  inline void ipv6_address::port(in_port_t p)
  {
    sin6_port = htons(p);
  }

  inline ipv6_address::operator const socket_address*() const
  {
    return reinterpret_cast<const socket_address*>(this);
  }

  inline ipv6_address::operator socket_address*()
  {
    return reinterpret_cast<socket_address*>(this);
  }

  inline ipv6_address::operator const socket_address&() const
  {
    return reinterpret_cast<const socket_address&>(*this);
  }

  inline ipv6_address::operator socket_address&()
  {
    return reinterpret_cast<socket_address&>(*this);
  }
}

#endif // NET_IPV6_ADDRESS_H
