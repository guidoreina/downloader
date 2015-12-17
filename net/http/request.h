#ifndef NET_HTTP_REQUEST_H
#define NET_HTTP_REQUEST_H

#include <stdlib.h>
#include "net/socket_address.h"
#include "net/http/methods.h"
#include "net/uri/uri.h"
#include "string/buffer.h"
#include "fs/file.h"
#include "util/move.h"

namespace net {
  namespace http {
    class request {
      public:
        // Constructor.
        request();

        // Destructor.
        ~request();

        // Initialize.
        bool init(const socket_address& addr,
                  method method,
                  const uri::uri& uri);

        void init(const socket_address& addr,
                  method method,
                  uri::uri&& uri);

        bool init(const socket_address& addr,
                  method method,
                  const uri::uri& uri,
                  const string::buffer* buf);

        void init(const socket_address& addr,
                  method method,
                  uri::uri&& uri,
                  const string::buffer* buf);

        bool init(const socket_address& addr,
                  method method,
                  const uri::uri& uri,
                  const char* filename);

        bool init(const socket_address& addr,
                  method method,
                  uri::uri&& uri,
                  const char* filename);

        // Clear.
        void clear();

        // Get address.
        const socket_address& address() const;

        // Get method.
        http::method method() const;

        // Get URI.
        const uri::uri& uri() const;

        // Get buffer.
        const string::buffer* buffer() const;

        // Get file.
        fs::file& file();

        // Get Content-Length.
        off_t content_length() const;

      private:
        socket_address _M_addr;
        http::method _M_method;
        uri::uri _M_uri;

        const string::buffer* _M_buf;
        fs::file _M_file;

        off_t _M_content_length;
    };

    inline request::request()
      : _M_buf(NULL),
        _M_content_length(0)
    {
    }

    inline request::~request()
    {
    }

    inline bool request::init(const socket_address& addr,
                              http::method method,
                              const uri::uri& uri)
    {
      if (!_M_uri.init(uri)) {
        return false;
      }

      _M_addr = addr;
      _M_method = method;

      return true;
    }

    inline void request::init(const socket_address& addr,
                              http::method method,
                              uri::uri&& uri)
    {
      _M_addr = addr;
      _M_method = method;
      _M_uri = util::move(uri);
    }

    inline bool request::init(const socket_address& addr,
                              http::method method,
                              const uri::uri& uri,
                              const string::buffer* buf)
    {
      if (!_M_uri.init(uri)) {
        return false;
      }

      _M_addr = addr;
      _M_method = method;
      _M_buf = buf;
      _M_content_length = buf->length();

      return true;
    }

    inline void request::init(const socket_address& addr,
                              http::method method,
                              uri::uri&& uri,
                              const string::buffer* buf)
    {
      _M_addr = addr;
      _M_method = method;
      _M_uri = util::move(uri);
      _M_buf = buf;
      _M_content_length = buf->length();
    }

    inline bool request::init(const socket_address& addr,
                              http::method method,
                              const uri::uri& uri,
                              const char* filename)
    {
      if (!_M_uri.init(uri)) {
        return false;
      }

      if (!_M_file.open(filename, O_RDONLY)) {
        return false;
      }

      if (((_M_content_length =
            _M_file.seek(0, fs::file::whence::kSeekEnd)) < 0) ||
          (_M_file.seek(0, fs::file::whence::kSeekSet) < 0)) {
        return false;
      }

      _M_addr = addr;
      _M_method = method;

      return true;
    }

    inline bool request::init(const socket_address& addr,
                              http::method method,
                              uri::uri&& uri,
                              const char* filename)
    {
      if (!_M_file.open(filename, O_RDONLY)) {
        return false;
      }

      if (((_M_content_length =
            _M_file.seek(0, fs::file::whence::kSeekEnd)) < 0) ||
          (_M_file.seek(0, fs::file::whence::kSeekSet) < 0)) {
        return false;
      }

      _M_addr = addr;
      _M_method = method;
      _M_uri = util::move(uri);

      return true;
    }

    inline void request::clear()
    {
      _M_uri.clear();

      _M_buf = NULL;
      _M_file.close();

      _M_content_length = 0;
    }

    inline const socket_address& request::address() const
    {
      return _M_addr;
    }

    inline method request::method() const
    {
      return _M_method;
    }

    inline const uri::uri& request::uri() const
    {
      return _M_uri;
    }

    inline const string::buffer* request::buffer() const
    {
      return _M_buf;
    }

    inline fs::file& request::file()
    {
      return _M_file;
    }

    inline off_t request::content_length() const
    {
      return _M_content_length;
    }
  }
}

#endif // NET_HTTP_REQUEST_H
