#ifndef NET_TCP_CONNECTION_H
#define NET_TCP_CONNECTION_H

#include <sys/types.h>
#include "io/event_handler.h"
#include "timer/event_handler.h"
#include "timer/timer.h"
#include "net/socket.h"

#if HAVE_SSL
  #include "net/ssl_socket.h"
#endif

#include "string/buffer.h"
#include "fs/file.h"
#include "util/ranges.h"

namespace net {
  class tcp_connection : public io::event_handler,
                         public timer::event_handler {
    public:
      // Constructor.
      tcp_connection();

      // Destructor.
      virtual ~tcp_connection();

      // Free.
      virtual void free();

      // Clear.
      virtual void clear();

      // SSL enabled?
      bool ssl() const;

#if HAVE_SSL
      // Enable/disable SSL.
      void ssl(bool enabled);
#endif // HAVE_SSL

      // On I/O.
      io::event_handler::result on_io(io::event events);

      // On timer.
      virtual bool on_timer() = 0;

      // Connect.
      io::event_handler::result connect(const socket_address& addr);

#if HAVE_SSL
      // Perform TLS/SSL handshake.
      io::event_handler::result handshake(ssl_socket::ssl_mode mode);
#endif // HAVE_SSL

      // Read.
      io::event_handler::result read(string::buffer& buf, size_t& count);
      io::event_handler::result read(size_t& count);
      io::event_handler::result read();

      // Write.
      io::event_handler::result write(const string::buffer& buf);
      io::event_handler::result write();

      // Write from multiple buffers.
      io::event_handler::result writev(const string::buffer** bufs,
                                       unsigned count);

      // Set file offset.
      void set_file_offset(off_t off);

      // Send file.
      io::event_handler::result sendfile(fs::file& f,
                                         off_t filesize,
                                         const util::range* range);

      io::event_handler::result sendfile(fs::file& f, off_t filesize);

      // Run.
      virtual io::event_handler::result run() = 0;

      // Readable?
      bool readable() const;

      // Writable?
      bool writable() const;

      // Get socket descriptor.
      int fd() const;

      // Set socket descriptor.
      void fd(int descriptor);

      // Get timer.
      timer::timer* timer();

    protected:
      static const size_t kReadBufferSize = 2 * 1024;

      socket _M_socket;

#if HAVE_SSL
      ssl_socket _M_ssl_socket;
      string::buffer _M_ssl_buf;
#endif // HAVE_SSL

      timer::timer _M_timer;

      string::buffer _M_in;
      string::buffer _M_out;

      off_t _M_inp;
      off_t _M_outp;

#if HAVE_SSL
      off_t _M_file_offset;
#endif

      unsigned _M_readable:1;
      unsigned _M_writable:1;

#if HAVE_SSL
      unsigned _M_ssl:1;
#endif

    private:
      struct operations {
        io::event_handler::result (*read)(tcp_connection* conn,
                                          string::buffer& buf,
                                          size_t& count);

        io::event_handler::result (*write)(tcp_connection* conn,
                                           const string::buffer& buf);

        io::event_handler::result (*writev)(tcp_connection* conn,
                                            const string::buffer** bufs,
                                            unsigned count);

        io::event_handler::result (*sendfile)(tcp_connection* conn,
                                              fs::file& f,
                                              off_t filesize,
                                              const util::range* range);
      };

      static const operations _M_operations;

#if HAVE_SSL
      static const operations _M_ssl_operations;
#endif

      const operations* _M_current_operations;

      // Read.
      static io::event_handler::result _M_read(tcp_connection* conn,
                                               string::buffer& buf,
                                               size_t& count);

      // Write.
      static io::event_handler::result _M_write(tcp_connection* conn,
                                                const string::buffer& buf);

      // Write from multiple buffers.
      static io::event_handler::result _M_writev(tcp_connection* conn,
                                                 const string::buffer** bufs,
                                                 unsigned count);

      // Send file.
      static io::event_handler::result _M_sendfile(tcp_connection* conn,
                                                   fs::file& f,
                                                   off_t filesize,
                                                   const util::range* range);

#if HAVE_SSL
      // Read.
      static io::event_handler::result _M_ssl_read(tcp_connection* conn,
                                                   string::buffer& buf,
                                                   size_t& count);

      // Write.
      static io::event_handler::result _M_ssl_write(tcp_connection* conn,
                                                    const string::buffer& buf);

      // Write from multiple buffers.
      static
      io::event_handler::result _M_ssl_writev(tcp_connection* conn,
                                              const string::buffer** bufs,
                                              unsigned count);

      // Send file.
      static
      io::event_handler::result _M_ssl_sendfile(tcp_connection* conn,
                                                fs::file& f,
                                                off_t filesize,
                                                const util::range* range);
#endif // HAVE_SSL

      // Disable copy constructor and assignment operator.
      tcp_connection(const tcp_connection&) = delete;
      tcp_connection& operator=(const tcp_connection&) = delete;
  };

  inline tcp_connection::~tcp_connection()
  {
#if HAVE_SSL
    _M_ssl_socket.free();
#endif // HAVE_SSL
  }

  inline void tcp_connection::free()
  {
    tcp_connection::clear();

#if HAVE_SSL
    _M_ssl_socket.free();
    _M_ssl_buf.clear();
#endif // HAVE_SSL

    _M_in.clear();
    _M_inp = 0;

    _M_readable = 0;
    _M_writable = 0;

#if HAVE_SSL
    _M_ssl = 0;
    _M_current_operations = &_M_operations;
#endif // HAVE_SSL
  }

  inline void tcp_connection::clear()
  {
    _M_out.clear();
    _M_outp = 0;

#if HAVE_SSL
    _M_file_offset = 0;
#endif
  }

  inline bool tcp_connection::ssl() const
  {
#if !HAVE_SSL
    return false;
#else
    return _M_ssl;
#endif
  }

#if HAVE_SSL
  inline void tcp_connection::ssl(bool enabled)
  {
    if ((_M_ssl = enabled) == true) {
      _M_current_operations = &_M_ssl_operations;
    } else {
      _M_current_operations = &_M_operations;
    }
  }
#endif // HAVE_SSL

  inline io::event_handler::result tcp_connection::on_io(io::event events)
  {
    if (static_cast<unsigned>(events) &
        static_cast<unsigned>(io::event::kRead)) {
      _M_readable = 1;
    }

    if (static_cast<unsigned>(events) &
        static_cast<unsigned>(io::event::kWrite)) {
      _M_writable = 1;
    }

    return run();
  }

  inline io::event_handler::result tcp_connection::read(string::buffer& buf,
                                                        size_t& count)
  {
    return _M_current_operations->read(this, buf, count);
  }

  inline io::event_handler::result tcp_connection::read(size_t& count)
  {
    return _M_current_operations->read(this, _M_in, count);
  }

  inline io::event_handler::result tcp_connection::read()
  {
    size_t count;
    return _M_current_operations->read(this, _M_in, count);
  }

  inline
  io::event_handler::result tcp_connection::write(const string::buffer& buf)
  {
    return _M_current_operations->write(this, buf);
  }

  inline io::event_handler::result tcp_connection::write()
  {
    return _M_current_operations->write(this, _M_out);
  }

  inline io::event_handler::result
  tcp_connection::writev(const string::buffer** bufs, unsigned count)
  {
    return _M_current_operations->writev(this, bufs, count);
  }

  inline void tcp_connection::set_file_offset(off_t off)
  {
#if !HAVE_SSL
    _M_outp = off;
#else
    if (!_M_ssl) {
      _M_outp = off;
    } else {
      _M_outp = 0;
      _M_file_offset = off;
    }
#endif
  }

  inline
  io::event_handler::result tcp_connection::sendfile(fs::file& f,
                                                     off_t filesize,
                                                     const util::range* range)
  {
    return _M_current_operations->sendfile(this, f, filesize, range);
  }

  inline io::event_handler::result tcp_connection::sendfile(fs::file& f,
                                                            off_t filesize)
  {
    return _M_current_operations->sendfile(this, f, filesize, NULL);
  }

  inline bool tcp_connection::readable() const
  {
    return _M_readable;
  }

  inline bool tcp_connection::writable() const
  {
    return _M_writable;
  }

  inline int tcp_connection::fd() const
  {
    return _M_socket.fd();
  }

  inline void tcp_connection::fd(int descriptor)
  {
    _M_socket.fd(descriptor);

#if HAVE_SSL
    if (_M_ssl) {
      _M_ssl_socket.fd(descriptor);
    }
#endif // HAVE_SSL
  }

  inline timer::timer* tcp_connection::timer()
  {
    return &_M_timer;
  }
}

#endif // NET_TCP_CONNECTION_H
