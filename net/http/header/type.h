#ifndef NET_HTTP_HEADER_TYPE_H
#define NET_HTTP_HEADER_TYPE_H

namespace net {
  namespace http {
    namespace header {
      enum class type {
        kGeneral,
        kRequest,
        kResponse,
        kEntity
      };
    }
  }
}

#endif // NET_HTTP_HEADER_TYPE_H
