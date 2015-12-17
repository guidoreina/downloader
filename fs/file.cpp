#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include "fs/file.h"

bool fs::file::open(const char* pathname, int flags)
{
  if ((_M_fd = ::open(pathname, flags)) < 0) {
    return false;
  }

  return true;
}

bool fs::file::open(const char* pathname, int flags, mode_t mode)
{
  if ((_M_fd = ::open(pathname, flags, mode)) < 0) {
    return false;
  }

  return true;
}

bool fs::file::close()
{
  if (_M_fd != -1) {
    if (::close(_M_fd) < 0) {
      return false;
    }

    _M_fd = -1;
  }

  return true;
}

ssize_t fs::file::read(void* buf, size_t count)
{
  ssize_t ret;

  do {
    ret = ::read(_M_fd, buf, count);
  } while ((ret < 0) && (errno == EINTR));

  return ret;
}

ssize_t fs::file::pread(void* buf, size_t count, off_t offset)
{
#if HAVE_PREAD
  ssize_t ret;

  do {
    ret = ::pread(_M_fd, buf, count, offset);
  } while ((ret < 0) && (errno == EINTR));

  return ret;
#else
  if (lseek(_M_fd, offset, SEEK_SET) != offset) {
    return -1;
  }

  return read(buf, count);
#endif
}

ssize_t fs::file::readv(const struct iovec* iov, unsigned iovcnt)
{
  ssize_t ret;

  do {
    ret = ::readv(_M_fd, iov, iovcnt);
  } while ((ret < 0) && (errno == EINTR));

  return ret;
}

bool fs::file::read_all(const char* pathname, string::buffer& buf, off_t max)
{
  struct stat status;
  if ((::stat(pathname, &status) < 0) ||
      (!S_ISREG(status.st_mode)) ||
      (status.st_size > max)) {
    return false;
  }

  // If the file is empty...
  if (status.st_size == 0) {
    return true;
  }

  if (!buf.allocate(status.st_size)) {
    return false;
  }

  file f;
  if (!f.open(pathname, O_RDONLY)) {
    return false;
  }

  char* end = buf.end();
  off_t size = status.st_size;

  do {
    ssize_t ret;
    if ((ret = f.read(end, size)) < 0) {
      return false;
    } else if (ret == 0) {
      break;
    } else {
      end += ret;
      size -= ret;
    }
  } while (size > 0);

  buf.increment_length(status.st_size - size);

  return true;
}

ssize_t fs::file::write(const void* buf, size_t count)
{
  const uint8_t* b = reinterpret_cast<const uint8_t*>(buf);
  size_t written = 0;

  do {
    ssize_t ret;
    if ((ret = ::write(_M_fd, b, count - written)) < 0) {
      if (errno != EINTR) {
        return -1;
      }
    } else if (ret > 0) {
      if ((written += ret) == count) {
        return count;
      }

      b += ret;
    }
  } while (true);
}

ssize_t fs::file::pwrite(const void* buf, size_t count, off_t offset)
{
#if HAVE_PWRITE
  const uint8_t* b = reinterpret_cast<const uint8_t*>(buf);
  size_t written = 0;

  do {
    ssize_t ret;
    if ((ret = ::pwrite(_M_fd, b, count - written, offset)) < 0) {
      if (errno != EINTR) {
        return -1;
      }
    } else if (ret > 0) {
      if ((written += ret) == count) {
        return count;
      }

      b += ret;
      offset += ret;
    }
  } while (true);
#else
  if (lseek(_M_fd, offset, SEEK_SET) != offset) {
    return -1;
  }

  return write(buf, count);
#endif
}

ssize_t fs::file::writev(const struct iovec* iov, unsigned iovcnt)
{
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
      if (errno != EINTR) {
        return -1;
      }
    } else if (ret > 0) {
      if ((written += ret) == total) {
        return total;
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

bool fs::file::truncate(off_t length)
{
  int ret;

  do {
    ret = ftruncate(_M_fd, length);
  } while ((ret < 0) && (errno == EINTR));

  return (ret == 0);
}
