#ifndef FS_FILE_H
#define FS_FILE_H

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include "string/buffer.h"

namespace fs {
  class file {
    public:
      enum class whence {
        kSeekSet = SEEK_SET,
        kSeekCur = SEEK_CUR,
        kSeekEnd = SEEK_END
      };

      // Constructor.
      file();
      file(int fd);

      // Destructor.
      ~file();

      // Open file.
      bool open(const char* pathname, int flags);
      bool open(const char* pathname, int flags, mode_t mode);

      // Close file.
      bool close();

      // Is open?
      bool is_open() const;

      // Read.
      ssize_t read(void* buf, size_t count);

      // Read at a given offset.
      ssize_t pread(void* buf, size_t count, off_t offset);

      // Read into multiple buffers.
      ssize_t readv(const struct iovec* iov, unsigned iovcnt);

      // Read file.
      static bool read_all(const char* pathname,
                           string::buffer& buf,
                           off_t max = 1024 * 1024);

      // Write.
      ssize_t write(const void* buf, size_t count);

      // Write at a given offset.
      ssize_t pwrite(const void* buf, size_t count, off_t offset);

      // Write from multiple buffers.
      ssize_t writev(const struct iovec* iov, unsigned iovcnt);

      // Seek.
      off_t seek(off_t offset, whence whence);

      // Get offset.
      off_t offset() const;

      // Get file status.
      bool stat(struct stat& buf) const;

      // Truncate file.
      bool truncate(off_t length);

      // Get file descriptor.
      int fd() const;

    private:
      int _M_fd;

      // Disable copy constructor and assignment operator.
      file(const file&);
      file& operator=(const file&);
  };

  inline file::file()
    : _M_fd(-1)
  {
  }

  inline file::file(int fd)
    : _M_fd(fd)
  {
  }

  inline file::~file()
  {
    close();
  }

  inline bool file::is_open() const
  {
    return (_M_fd != -1);
  }

  inline off_t file::seek(off_t offset, whence whence)
  {
    return lseek(_M_fd, offset, static_cast<int>(whence));
  }

  inline off_t file::offset() const
  {
    return lseek(_M_fd, 0, SEEK_CUR);
  }

  inline bool file::stat(struct stat& buf) const
  {
    return (fstat(_M_fd, &buf) == 0);
  }

  inline int file::fd() const
  {
    return _M_fd;
  }
}

#endif // FS_FILE_H
