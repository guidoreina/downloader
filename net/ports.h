#ifndef NET_PORTS_H
#define NET_PORTS_H

#include <netinet/in.h>

namespace net {
  in_port_t standard_port(const char* scheme, size_t len);
}

#endif // NET_PORTS_H
