#ifndef NET_HTTP_METHODS_H
#define NET_HTTP_METHODS_H

#include <stdint.h>
#include "string/slice.h"

namespace net {
  namespace http {
    enum class method : uint8_t {
      kConnect   = 0,
      kCopy      = 1,
      kDelete    = 2,
      kGet       = 3,
      kHead      = 4,
      kLock      = 5,
      kMkcol     = 6,
      kMove      = 7,
      kOptions   = 8,
      kPost      = 9,
      kPropfind  = 10,
      kProppatch = 11,
      kPut       = 12,
      kTrace     = 13,
      kUnlock    = 14,
      kUnknown   = 0xff
    };

    class methods {
      public:
        static method search(const char* s, size_t len);
        static string::slice name(method m);

      private:
        struct name {
          const char* name;
          size_t len;
        };

        static const struct name _M_names[];
    };

    inline string::slice methods::name(method m)
    {
      return string::slice(_M_names[static_cast<unsigned>(m)].name,
                           _M_names[static_cast<unsigned>(m)].len);
    }
  }
}

#endif // NET_HTTP_METHODS_H
