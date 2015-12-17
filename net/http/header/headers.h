#ifndef NET_HTTP_HEADER_HEADERS_H
#define NET_HTTP_HEADER_HEADERS_H

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "net/http/date.h"
#include "net/http/header/permanent_header.h"
#include "net/http/header/non_permanent_header.h"
#include "util/number.h"
#include "util/ranges.h"
#include "string/buffer.h"
#include "string/slice.h"
#include "macros/macros.h"

namespace net {
  namespace http {
    namespace header {
      class headers {
        public:
          static const size_t kHeadersMaxLen = 64 * 1024;
          static const size_t kBoundaryLen = 11;
          static const unsigned kMaxRanges = 16;

          // Constructor.
          headers(bool ignore_errors = true);

          // Destructor.
          ~headers();

          // Clear.
          void clear();

          // Reset.
          void reset();

          // Get header.
          string::slice header(permanent_field_name name) const;
          string::slice header(const char* name, size_t len) const;
          bool header(permanent_field_name name, int32_t& n) const;
          bool header(permanent_field_name name, uint32_t& n) const;
          bool header(permanent_field_name name, int64_t& n) const;
          bool header(permanent_field_name name, uint64_t& n) const;

          // Get timestamp.
          bool timestamp(permanent_field_name name,
                         time_t& t,
                         struct tm& tm) const;

          bool timestamp(permanent_field_name name, time_t& t) const;

          // Get ranges.
          bool ranges(off_t filesize, util::ranges& ranges) const;

          // Add header.
          bool add(permanent_field_name name,
                   const char* value,
                   size_t valuelen,
                   bool clean_value = false);

          bool add(permanent_field_name name, int64_t n);
          bool add(permanent_field_name name, uint64_t n);
          bool add(const char* name,
                   size_t namelen,
                   const char* value,
                   size_t valuelen,
                   bool clean_value = false);

          // Add timestamp.
          bool add_timestamp(permanent_field_name name, const struct tm& tm);
          bool add_timestamp(permanent_field_name name, time_t t);

          // Remove header.
          bool remove(permanent_field_name name);
          bool remove(const char* name, size_t namelen);

          // Parse.
          enum class parse_result {
            kInvalidHeader,
            kHeadersTooLarge,
            kNotEndOfHeader,
            kEndOfHeader
          };

          parse_result parse(const void* buf, size_t len);

          // Serialize.
          bool serialize(string::buffer& buf) const;

          // Get header size.
          size_t size() const;

        private:
          permanent_headers _M_permanent_headers[4];
          non_permanent_headers _M_non_permanent_headers;
          uint64_t _M_headers; // Bitmap of headers.

          string::buffer _M_buf;

          struct state {
            permanent_field_name header;

            size_t name;
            size_t namelen;

            size_t value;
            size_t value_end;

            size_t size;

            int state;

            void reset();
          };

          state _M_state;

          bool _M_ignore_errors;

          // Add timestamp.
          bool _M_add_timestamp(permanent_field_name name, const struct tm& tm);

          // Clean.
          static int _M_clean(const char* value, size_t valuelen, char* dest);

          // Disable copy constructor and assignment operator.
          headers(const headers&) = delete;
          headers& operator=(const headers&) = delete;
      };

      inline headers::headers(bool ignore_errors)
        : _M_permanent_headers{{_M_buf}, {_M_buf}, {_M_buf}, {_M_buf}},
          _M_non_permanent_headers(_M_buf),
          _M_headers(0),
          _M_ignore_errors(ignore_errors)
      {
        _M_state.reset();
      }

      inline headers::~headers()
      {
      }

      inline void headers::clear()
      {
        for (size_t i = 0; i < ARRAY_SIZE(_M_permanent_headers); i++) {
          _M_permanent_headers[i].clear();
        }

        _M_non_permanent_headers.clear();
        _M_headers = 0;

        _M_buf.free();

        _M_state.reset();
      }

      inline void headers::reset()
      {
        for (size_t i = 0; i < ARRAY_SIZE(_M_permanent_headers); i++) {
          _M_permanent_headers[i].reset();
        }

        _M_non_permanent_headers.reset();
        _M_headers = 0;

        _M_buf.clear();

        _M_state.reset();
      }

      inline string::slice headers::header(permanent_field_name name) const
      {
        // If the header has not been added...
        if (!(_M_headers & (static_cast<uint64_t>(1) <<
                            static_cast<unsigned>(name)))) {
          return string::slice();
        }

        return _M_permanent_headers[static_cast<size_t>(
                                      permanent_header::header_type(name)
                                    )].find(name);
      }

      inline string::slice headers::header(const char* name, size_t len) const
      {
        return _M_non_permanent_headers.find(name, len);
      }

      inline bool headers::header(permanent_field_name name, int32_t& n) const
      {
        // If the header has not been added...
        if (!(_M_headers & (static_cast<uint64_t>(1) <<
                            static_cast<unsigned>(name)))) {
          return false;
        }

        string::slice s(_M_permanent_headers[static_cast<size_t>(
                                               permanent_header::header_type(
                                                 name
                                               )
                                             )].find(name));

        return (util::number::parse(s.data(), s.length(), n) ==
                util::number::parse_result::kSucceeded);
      }

      inline bool headers::header(permanent_field_name name, uint32_t& n) const
      {
        // If the header has not been added...
        if (!(_M_headers & (static_cast<uint64_t>(1) <<
                            static_cast<unsigned>(name)))) {
          return false;
        }

        string::slice s(_M_permanent_headers[static_cast<size_t>(
                                               permanent_header::header_type(
                                                 name
                                               )
                                             )].find(name));

        return (util::number::parse(s.data(), s.length(), n) ==
                util::number::parse_result::kSucceeded);
      }

      inline bool headers::header(permanent_field_name name, int64_t& n) const
      {
        // If the header has not been added...
        if (!(_M_headers & (static_cast<uint64_t>(1) <<
                            static_cast<unsigned>(name)))) {
          return false;
        }

        string::slice s(_M_permanent_headers[static_cast<size_t>(
                                               permanent_header::header_type(
                                                 name
                                               )
                                             )].find(name));

        return (util::number::parse(s.data(), s.length(), n) ==
                util::number::parse_result::kSucceeded);
      }

      inline bool headers::header(permanent_field_name name, uint64_t& n) const
      {
        // If the header has not been added...
        if (!(_M_headers & (static_cast<uint64_t>(1) <<
                            static_cast<unsigned>(name)))) {
          return false;
        }

        string::slice s(_M_permanent_headers[static_cast<size_t>(
                                               permanent_header::header_type(
                                                 name
                                               )
                                             )].find(name));

        return (util::number::parse(s.data(), s.length(), n) ==
                util::number::parse_result::kSucceeded);
      }

      inline bool headers::timestamp(permanent_field_name name,
                                     time_t& t,
                                     struct tm& tm) const
      {
        // If the header has not been added...
        if (!(_M_headers & (static_cast<uint64_t>(1) <<
                            static_cast<unsigned>(name)))) {
          return false;
        }

        string::slice s(_M_permanent_headers[static_cast<size_t>(
                                               permanent_header::header_type(
                                                 name
                                               )
                                             )].find(name));

        return ((t = date::parse(s.data(), s.length(), tm)) !=
                static_cast<time_t>(-1));
      }

      inline bool headers::timestamp(permanent_field_name name, time_t& t) const
      {
        struct tm tm;
        return timestamp(name, t, tm);
      }

      inline bool headers::add_timestamp(permanent_field_name name,
                                         const struct tm& tm)
      {
        // If the header has been added already...
        if (_M_headers & (static_cast<uint64_t>(1) <<
                          static_cast<unsigned>(name))) {
          // If the header might appear only once...
          if (!permanent_header::list(name)) {
            return _M_ignore_errors;
          }
        }

        // If the headers are too large...
        if (_M_buf.length() >= kHeadersMaxLen) {
          return false;
        }

        return _M_add_timestamp(name, tm);
      }

      inline bool headers::remove(const char* name, size_t namelen)
      {
        return ((_M_non_permanent_headers.remove(name, namelen)) ||
                (_M_ignore_errors));
      }

      inline size_t headers::size() const
      {
        return _M_state.size;
      }

      inline void headers::state::reset()
      {
        namelen = 0;
        value_end = 0;
        size = 0;
        state = 0;
      }
    }
  }
}

#endif // NET_HTTP_HEADER_HEADERS_H
