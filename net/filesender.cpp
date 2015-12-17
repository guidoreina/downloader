#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include "net/filesender.h"
#include "macros/macros.h"

#if HAVE_SENDFILE
  #if defined(__linux__) || defined(__sun__)
    #include <sys/sendfile.h>

    off_t net::filesender::sendfile(socket& s,
                                    fs::file& f,
                                    off_t filesize,
                                    off_t& offset,
                                    off_t count,
                                    int timeout)
    {
      if (timeout == 0) {
        ssize_t ret;
        while (((ret = ::sendfile(s.fd(), f.fd(), &offset, count)) < 0) &&
               (errno == EINTR));

        return ret;
      } else {
        off_t written = 0;

        do {
          ssize_t ret;
          if ((ret = ::sendfile(s.fd(),
                                f.fd(),
                                &offset,
                                count - written)) < 0) {
            switch (errno) {
              case EAGAIN:
                if (!s.wait_writable(timeout)) {
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

            if (!s.wait_writable(timeout)) {
              return -1;
            }
          }
        } while (true);
      }
    }
  #elif defined(__FreeBSD__) || \
        defined(__OpenBSD__) || \
        defined(__NetBSD__) || \
        defined(__DragonFly__)
    #include <sys/uio.h>

    off_t net::filesender::sendfile(socket& s,
                                    fs::file& f,
                                    off_t filesize,
                                    off_t& offset,
                                    off_t count,
                                    int timeout)
    {
      off_t written = 0;
      off_t sbytes;

      if (timeout == 0) {
        while (::sendfile(f.fd(),
                          s.fd(),
                          offset,
                          count - written,
                          NULL,
                          &sbytes,
                          0) < 0) {
          switch (errno) {
              case EAGAIN:
                offset += sbytes;
                written += sbytes;

                return written;
              case EINTR:
                offset += sbytes;
                written += sbytes;

                break;
              default:
                return -1;
          }
        }

        offset += sbytes;
        written += sbytes;

        return written;
      } else {
        do {
          if (::sendfile(f.fd(),
                         s.fd(),
                         offset,
                         count - written,
                         NULL,
                         &sbytes,
                         0) < 0) {
            switch (errno) {
              case EAGAIN:
                if (!s.wait_writable(timeout)) {
                  return -1;
                }

                offset += sbytes;
                written += sbytes;

                break;
              case EINTR:
                offset += sbytes;
                written += sbytes;

                break;
              default:
                return -1;
            }
          } else {
            offset += sbytes;

            if ((written += sbytes) == count) {
              return count;
            }

            if (!s.wait_writable(timeout)) {
              return -1;
            }
          }
        } while (true);
      }
    }
  #endif
#elif HAVE_MMAP
  #include <sys/mman.h>

  off_t net::filesender::sendfile(socket& s,
                                  fs::file& f,
                                  off_t filesize,
                                  off_t& offset,
                                  off_t count,
                                  int timeout)
  {
    int pagesize = getpagesize();

    off_t written = 0;

    do {
      off_t off = (offset / pagesize) * pagesize;
      off_t length = MIN(filesize - off,
                         static_cast<off_t>(kMmapMappingSize));

      void* data;
      if ((data = mmap(NULL,
                       length,
                       PROT_READ,
                       MAP_SHARED,
                       f.fd(),
                       off)) == MAP_FAILED) {
        return -1;
      }

      size_t n = MIN(count - written, off + length - offset);

      ssize_t ret = s.write(reinterpret_cast<const uint8_t*>(data) +
                            (offset - off),
                            n,
                            timeout);

      // Save 'errno'.
      int error = errno;

      munmap(data, length);

      // Restore 'errno'.
      errno = error;

      if (ret < 0) {
        return (error == EAGAIN) ? written : -1;
      }

      offset += ret;

      if ((written += ret) == count) {
        return count;
      }

      // If we couldn't send all the data...
      if (static_cast<size_t>(ret) < n) {
        return written;
      }
    } while (true);
  }
#else
  off_t net::filesender::sendfile(socket& s,
                                  fs::file& f,
                                  off_t filesize,
                                  off_t& offset,
                                  off_t count,
                                  int timeout)
  {
  #if !HAVE_PREAD
    if (f.seek(offset, fs::file::whence::kSeekSet) != offset) {
      return -1;
    }
  #endif // !HAVE_PREAD

    char buf[kReadBufferSize];

    off_t written = 0;

    if (timeout == 0) {
      do {
        off_t bytes = MIN(static_cast<off_t>(sizeof(buf)), count - written);

    #if HAVE_PREAD
        if ((bytes = f.pread(buf, bytes, offset)) <= 0) {
    #else
        if ((bytes = f.read(buf, bytes)) <= 0) {
    #endif
          return -1;
        }

        ssize_t ret;
        if ((ret = s.write(buf, bytes, 0)) < 0) {
          if ((errno == EAGAIN) && (written > 0)) {
            return written;
          }

          return -1;
        } else if (ret == 0) {
          return written;
        } else {
          offset += ret;

          if ((written += ret) == count) {
            return count;
          }

          // If we couldn't send all the data...
          if (ret < bytes) {
            return written;
          }
        }
      } while (true);
    } else {
      do {
        off_t bytes = MIN(static_cast<off_t>(sizeof(buf)), count - written);

    #if HAVE_PREAD
        if ((bytes = f.pread(buf, bytes, offset)) <= 0) {
    #else
        if ((bytes = f.read(buf, bytes)) <= 0) {
    #endif
          return -1;
        }

        ssize_t ret;
        if ((ret = s.write(buf, bytes, timeout)) < 0) {
          return -1;
        } else {
          offset += ret;

          if ((written += ret) == count) {
            return count;
          }
        }
      } while (true);
    }
  }
#endif
