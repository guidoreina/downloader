#ifndef NET_HTTP_HEADER_NON_PERMANENT_HEADER_H
#define NET_HTTP_HEADER_NON_PERMANENT_HEADER_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "string/buffer.h"
#include "string/slice.h"

namespace net {
  namespace http {
    namespace header {
      class non_permanent_header {
        friend class non_permanent_headers;

        public:
          // Constructor.
          non_permanent_header();

          // Initialize.
          void init(size_t nameoff,
                    size_t namelen,
                    size_t valueoff,
                    size_t valuelen);

        private:
          size_t _M_nameoff;
          size_t _M_namelen;
          size_t _M_valueoff;
          size_t _M_valuelen;
      };

      class non_permanent_headers {
        public:
          static const size_t kMaxHeaders = 100;

          // Constructor.
          non_permanent_headers(const string::buffer& buf);

          // Destructor.
          ~non_permanent_headers();

          // Clear.
          void clear();

          // Reset.
          void reset();

          // Add non-permanent header.
          bool add(size_t nameoff,
                   size_t namelen,
                   size_t valueoff,
                   size_t valuelen);

          // Remove non-permanent header.
          bool remove(const char* s, size_t len);

          // Find non-permanent header.
          string::slice find(const char* s, size_t len) const;

          // Get non-permanent header.
          bool get(size_t idx, string::slice& name, string::slice& value) const;

        private:
          static const uint16_t kInitialAlloc = 4;

          const string::buffer& _M_buf;

          non_permanent_header* _M_headers;
          uint16_t _M_size;
          uint16_t _M_used;

          // Disable copy constructor and assignment operator.
          non_permanent_headers(const non_permanent_headers&) = delete;

          non_permanent_headers&
          operator=(const non_permanent_headers&) = delete;
      };

      inline non_permanent_header::non_permanent_header()
      {
      }

      inline void non_permanent_header::init(size_t nameoff,
                                             size_t namelen,
                                             size_t valueoff,
                                             size_t valuelen)
      {
        _M_nameoff = nameoff;
        _M_namelen = namelen;
        _M_valueoff = valueoff;
        _M_valuelen = valuelen;
      }

      inline
      non_permanent_headers::non_permanent_headers(const string::buffer& buf)
        : _M_buf(buf),
          _M_headers(NULL),
          _M_size(0),
          _M_used(0)
      {
      }

      inline non_permanent_headers::~non_permanent_headers()
      {
        if (_M_headers) {
          free(_M_headers);
        }
      }

      inline void non_permanent_headers::clear()
      {
        if (_M_headers) {
          free(_M_headers);
          _M_headers = NULL;
        }

        _M_size = 0;
        _M_used = 0;
      }

      inline void non_permanent_headers::reset()
      {
        _M_used = 0;
      }

      inline
      string::slice non_permanent_headers::find(const char* s, size_t len) const
      {
        for (uint16_t i = 0; i < _M_used; i++) {
          const non_permanent_header* h = &_M_headers[i];
          if ((len == h->_M_namelen) &&
              (strncasecmp(s, _M_buf.data() + h->_M_nameoff, len) == 0)) {
            return string::slice(_M_buf.data() + h->_M_valueoff, h->_M_valuelen);
          }
        }

        return string::slice();
      }

      inline bool non_permanent_headers::get(size_t idx,
                                             string::slice& name,
                                             string::slice& value) const
      {
        if (idx < _M_used) {
          const non_permanent_header* h = &_M_headers[idx];

          name.set(_M_buf.data() + h->_M_nameoff, h->_M_namelen);
          value.set(_M_buf.data() + h->_M_valueoff, h->_M_valuelen);

          return true;
        } else {
          return false;
        }
      }
    }
  }
}

#endif // NET_HTTP_HEADER_NON_PERMANENT_HEADER_H
