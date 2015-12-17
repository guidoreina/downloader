#include <stdlib.h>
#include <stdint.h>

#if HAVE_MMAP
  #include <sys/mman.h>
#endif

#include <sys/socket.h>
#include <limits.h>
#include <errno.h>
#include "net/tcp_connection.h"
#include "net/filesender.h"
#include "macros/macros.h"

const net::tcp_connection::operations net::tcp_connection::_M_operations = {
  tcp_connection::_M_read,
  tcp_connection::_M_write,
  tcp_connection::_M_writev,
  tcp_connection::_M_sendfile
};

#if HAVE_SSL
  const
  net::tcp_connection::operations net::tcp_connection::_M_ssl_operations = {
    tcp_connection::_M_ssl_read,
    tcp_connection::_M_ssl_write,
    tcp_connection::_M_ssl_writev,
    tcp_connection::_M_ssl_sendfile
  };
#endif // HAVE_SSL

net::tcp_connection::tcp_connection()
  : _M_timer(this),
    _M_inp(0),
    _M_outp(0),
#if HAVE_SSL
    _M_file_offset(0),
#endif
    _M_readable(0),
    _M_writable(0),
#if HAVE_SSL
    _M_ssl(0),
#endif
    _M_current_operations(&_M_operations)
{
}

io::event_handler::result
net::tcp_connection::connect(const socket_address& addr)
{
  // Reset errno.
  errno = 0;

  if (!_M_socket.connect(socket::type::kStream, addr, 0)) {
    return io::event_handler::result::kError;
  }

  if (errno == EINPROGRESS) {
    _M_writable = 0;
    return io::event_handler::result::kChangeToWriteMode;
  }

  return io::event_handler::result::kSuccess;
}

#if HAVE_SSL
  io::event_handler::result
  net::tcp_connection::handshake(ssl_socket::ssl_mode mode)
  {
    // Handshake.
    bool readable = true, writable = true;
    if (!_M_ssl_socket.handshake(mode, readable, writable)) {
      if (!readable) {
        _M_readable = 0;
        return io::event_handler::result::kChangeToReadMode;
      } else if (!_M_writable) {
        _M_writable = 0;
        return io::event_handler::result::kChangeToWriteMode;
      } else {
        return io::event_handler::result::kError;
      }
    } else {
      return io::event_handler::result::kSuccess;
    }
  }
#endif // HAVE_SSL

io::event_handler::result net::tcp_connection::_M_read(tcp_connection* conn,
                                                       string::buffer& buf,
                                                       size_t& count)
{
  // Allocate memory.
  if (!buf.allocate(kReadBufferSize)) {
    return io::event_handler::result::kError;
  }

  // Receive.
  size_t remaining = buf.remaining();
  ssize_t ret;
  switch (ret = conn->_M_socket.read(buf.end(), remaining, 0)) {
    case -1:
      if (errno == EAGAIN) {
        count = 0;
        conn->_M_readable = 0;
      } else {
        return io::event_handler::result::kError;
      }

      break;
    case 0:
      // The peer has performed an orderly shutdown.
      return io::event_handler::result::kError;
    default:
      buf.increment_length(ret);
      count = ret;

      if (static_cast<size_t>(ret) < remaining) {
        conn->_M_readable = 0;
      }
  }

  return io::event_handler::result::kSuccess;
}

io::event_handler::result
net::tcp_connection::_M_write(tcp_connection* conn, const string::buffer& buf)
{
  // Send.
  size_t count = buf.length() - conn->_M_outp;
  ssize_t ret;
  if ((ret = conn->_M_socket.write(buf.data() + conn->_M_outp, count, 0)) < 0) {
    if (errno == EAGAIN) {
      conn->_M_writable = 0;
    } else {
      return io::event_handler::result::kError;
    }
  } else {
    conn->_M_outp += ret;

    if (static_cast<size_t>(ret) < count) {
      conn->_M_writable = 0;
    }
  }

  return io::event_handler::result::kSuccess;
}

io::event_handler::result
net::tcp_connection::_M_writev(tcp_connection* conn,
                               const string::buffer** bufs,
                               unsigned count)
{
  // Too many buffers?
  if (count > IOV_MAX) {
    errno = EINVAL;
    return io::event_handler::result::kError;
  }

  off_t outp = conn->_M_outp;
  while ((count > 0) && (outp >= static_cast<off_t>((*bufs)->length()))) {
    outp -= (*bufs)->length();

    bufs++;
    count--;
  }

  struct iovec vec[IOV_MAX];
  size_t total = 0;

  for (unsigned i = 0; i < count; i++) {
    vec[i].iov_base = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(
                                             (*bufs)->data()
                                           )
                                          ) + outp;

    vec[i].iov_len = (*bufs)->length() - outp;

    total += vec[i].iov_len;

    outp = 0;

    bufs++;
  }

  // Send.
  ssize_t ret;
  if ((ret = conn->_M_socket.writev(vec, count, 0)) < 0) {
    if (errno == EAGAIN) {
      conn->_M_writable = 0;
    } else {
      return io::event_handler::result::kError;
    }
  } else {
    conn->_M_outp += ret;

    if (static_cast<size_t>(ret) < total) {
      conn->_M_writable = 0;
    }
  }

  return io::event_handler::result::kSuccess;
}

io::event_handler::result
net::tcp_connection::_M_sendfile(tcp_connection* conn,
                                 fs::file& f,
                                 off_t filesize,
                                 const util::range* range)
{
  // Get the number of bytes to send.
  off_t count = !range ? (filesize - conn->_M_outp) :
                         (range->to - conn->_M_outp + 1);

  // Send file.
  off_t ret;
  if ((ret = filesender::sendfile(conn->_M_socket,
                                  f,
                                  filesize,
                                  conn->_M_outp,
                                  count,
                                  0)) < 0) {
    if (errno == EAGAIN) {
      conn->_M_writable = 0;
    } else {
      return io::event_handler::result::kError;
    }
  } else {
    if (ret < count) {
      conn->_M_writable = 0;
    }
  }

  return io::event_handler::result::kSuccess;
}

#if HAVE_SSL
  io::event_handler::result
  net::tcp_connection::_M_ssl_read(tcp_connection* conn,
                                   string::buffer& buf,
                                   size_t& count)
  {
    // Allocate memory.
    if (!buf.allocate(kReadBufferSize)) {
      return io::event_handler::result::kError;
    }

    count = 0;

    do {
      // Receive.
      bool readable = true, writable = true;
      ssize_t ret;
      if ((ret = conn->_M_ssl_socket.read(buf.end(),
                                          buf.remaining(),
                                          readable,
                                          writable)) < 0) {
        if (!readable) {
          conn->_M_readable = 0;
          return io::event_handler::result::kChangeToReadMode;
        } else if (!writable) {
          conn->_M_writable = 0;
          return io::event_handler::result::kChangeToWriteMode;
        } else {
          return io::event_handler::result::kError;
        }
      } else {
        buf.increment_length(ret);
        count += ret;

        if (buf.remaining() == 0) {
          return io::event_handler::result::kSuccess;
        }
      }
    } while (true);
  }

  io::event_handler::result
  net::tcp_connection::_M_ssl_write(tcp_connection* conn,
                                    const string::buffer& buf)
  {
    // Send.
    size_t count = buf.length() - conn->_M_outp;

    bool readable = true, writable = true;
    ssize_t ret;
    if ((ret = conn->_M_ssl_socket.write(buf.data() + conn->_M_outp,
                                         count,
                                         readable,
                                         writable)) < 0) {
      if (!readable) {
        conn->_M_readable = 0;
        return io::event_handler::result::kChangeToReadMode;
      } else if (!writable) {
        conn->_M_writable = 0;
        return io::event_handler::result::kChangeToWriteMode;
      } else {
        return io::event_handler::result::kError;
      }
    } else {
      conn->_M_outp += ret;

      return io::event_handler::result::kSuccess;
    }
  }

  io::event_handler::result
  net::tcp_connection::_M_ssl_writev(tcp_connection* conn,
                                     const string::buffer** bufs,
                                     unsigned count)
  {
    if (conn->_M_ssl_buf.length() == 0) {
      // Too many buffers?
      if (count > IOV_MAX) {
        errno = EINVAL;
        return io::event_handler::result::kError;
      }

      for (unsigned i = 0; i < count; i++) {
        if (!conn->_M_ssl_buf.append(bufs[i]->data(), bufs[i]->length())) {
          return io::event_handler::result::kError;
        }
      }
    }

    io::event_handler::result res = _M_ssl_write(conn, conn->_M_ssl_buf);
    if (res == io::event_handler::result::kSuccess) {
      conn->_M_ssl_buf.clear();
    }

    return res;
  }

  #if HAVE_MMAP
    io::event_handler::result
    net::tcp_connection::_M_ssl_sendfile(tcp_connection* conn,
                                         fs::file& f,
                                         off_t filesize,
                                         const util::range* range)
    {
      int pagesize = getpagesize();

      do {
        if (conn->_M_ssl_buf.length() == 0) {
          // Get the number of bytes to send.
          off_t count = !range ? (filesize - conn->_M_file_offset) :
                                 (range->to - conn->_M_file_offset + 1);

          if (count == 0) {
            // Set '_M_outp' to '_M_file_offset', so the caller knows
            // that all the data has been sent.
            conn->_M_outp = conn->_M_file_offset;

            return io::event_handler::result::kSuccess;
          }

          off_t written = 0;

          do {
            off_t off = (conn->_M_file_offset / pagesize) * pagesize;
            off_t length = MIN(filesize - off,
                               static_cast<off_t>(
                                 filesender::kMmapMappingSize
                               ));

            void* data;
            if ((data = mmap(NULL,
                             length,
                             PROT_READ,
                             MAP_SHARED,
                             f.fd(),
                             off)) == MAP_FAILED) {
              return io::event_handler::result::kError;
            }

            off_t left = off + length - conn->_M_file_offset;

            do {
              off_t bytes = MIN3(static_cast<off_t>(
                                   filesender::kReadBufferSize
                                 ),
                                 count - written,
                                 left);

              if (!conn->_M_ssl_buf.append(reinterpret_cast<const char*>(data) +
                                           (conn->_M_file_offset - off),
                                           bytes)) {
                munmap(data, length);
                return io::event_handler::result::kError;
              }

              io::event_handler::result res = _M_ssl_write(conn,
                                                           conn->_M_ssl_buf);

              if (res != io::event_handler::result::kSuccess) {
                munmap(data, length);
                return res;
              }

              conn->_M_ssl_buf.clear();

              conn->_M_file_offset += bytes;

              if ((written += bytes) == count) {
                munmap(data, length);

                // Set '_M_outp' to '_M_file_offset', so the caller knows
                // that all the data has been sent.
                conn->_M_outp = conn->_M_file_offset;

                return io::event_handler::result::kSuccess;
              }

              left -= bytes;

              conn->_M_outp = 0;
            } while (left > 0);

            munmap(data, length);
          } while (true);
        }

        io::event_handler::result res = _M_ssl_write(conn, conn->_M_ssl_buf);
        if (res != io::event_handler::result::kSuccess) {
          return res;
        }

        conn->_M_file_offset += conn->_M_ssl_buf.length();

        conn->_M_ssl_buf.clear();
        conn->_M_outp = 0;
      } while (true);
    }
  #else
    io::event_handler::result
    net::tcp_connection::_M_ssl_sendfile(tcp_connection* conn,
                                         fs::file& f,
                                         off_t filesize,
                                         const util::range* range)
    {
      do {
        if (conn->_M_ssl_buf.length() == 0) {
          // Get the number of bytes to send.
          off_t count = !range ? (filesize - conn->_M_file_offset) :
                                 (range->to - conn->_M_file_offset + 1);

          if (count == 0) {
            // Set '_M_outp' to '_M_file_offset', so the caller knows
            // that all the data has been sent.
            conn->_M_outp = conn->_M_file_offset;

            return io::event_handler::result::kSuccess;
          }

      #if !HAVE_PREAD
          if (f.seek(conn->_M_file_offset,
                     fs::file::whence::kSeekSet) != conn->_M_file_offset) {
            return io::event_handler::result::kError;
          }
      #endif // !HAVE_PREAD

          if (!conn->_M_ssl_buf.allocate(filesender::kReadBufferSize)) {
            return io::event_handler::result::kError;
          }

          off_t written = 0;

          do {
            off_t bytes = MIN(static_cast<off_t>(filesender::kReadBufferSize),
                              count - written);

      #if HAVE_PREAD
            if ((bytes = f.pread(conn->_M_ssl_buf.data(),
                                 bytes,
                                 conn->_M_file_offset)) <= 0) {
      #else
            if ((bytes = f.read(conn->_M_ssl_buf.data(), bytes)) <= 0) {
      #endif
              return io::event_handler::result::kError;
            }

            conn->_M_ssl_buf.length(bytes);

            io::event_handler::result res = _M_ssl_write(conn,
                                                         conn->_M_ssl_buf);

            if (res != io::event_handler::result::kSuccess) {
              return res;
            }

            conn->_M_ssl_buf.clear();

            conn->_M_file_offset += bytes;

            if ((written += bytes) == count) {
              // Set '_M_outp' to '_M_file_offset', so the caller knows
              // that all the data has been sent.
              conn->_M_outp = conn->_M_file_offset;

              return io::event_handler::result::kSuccess;
            }

            conn->_M_outp = 0;
          } while (true);
        }

        io::event_handler::result res = _M_ssl_write(conn, conn->_M_ssl_buf);
        if (res != io::event_handler::result::kSuccess) {
          return res;
        }

        conn->_M_file_offset += conn->_M_ssl_buf.length();

        conn->_M_ssl_buf.clear();
        conn->_M_outp = 0;
      } while (true);
    }
  #endif
#endif // HAVE_SSL
