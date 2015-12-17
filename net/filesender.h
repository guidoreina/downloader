#ifndef NET_FILESENDER_H
#define NET_FILESENDER_H

#include <sys/types.h>
#include "net/socket.h"
#include "fs/file.h"

namespace net {
  class filesender {
    public:
      static const size_t kReadBufferSize = 8 * 1024;
      static const size_t kMmapMappingSize = 64 * 1024;

      static off_t sendfile(socket& s,
                            fs::file& f,
                            off_t filesize,
                            off_t& offset,
                            off_t count,
                            int timeout = -1);
  };
}

#endif // NET_FILESENDER_H
