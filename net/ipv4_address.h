#ifndef NET_IPV4_ADDRESS_H
#define NET_IPV4_ADDRESS_H

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net/socket_address.h"

namespace net {
  class ipv4_address : public sockaddr_in {
    public:
      static const char* kAnyAddress;
      static const char* kLocalhost;

      // Constructor.
      ipv4_address();

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

  inline ipv4_address::ipv4_address()
  {
    sin_family = AF_INET;

#ifndef __minix
    memset(sin_zero, 0, sizeof(sin_zero));
#endif // !__minix
  }

  inline in_port_t ipv4_address::port() const
  {
    return ntohs(sin_port);
  }

  inline void ipv4_address::port(in_port_t p)
  {
    sin_port = htons(p);
  }

  inline ipv4_address::operator const socket_address*() const
  {
    return reinterpret_cast<const socket_address*>(this);
  }

  inline ipv4_address::operator socket_address*()
  {
    return reinterpret_cast<socket_address*>(this);
  }

  inline ipv4_address::operator const socket_address&() const
  {
    return reinterpret_cast<const socket_address&>(*this);
  }

  inline ipv4_address::operator socket_address&()
  {
    return reinterpret_cast<socket_address&>(*this);
  }
}

#endif // NET_IPV4_ADDRESS_H
