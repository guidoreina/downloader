#ifndef NET_HTTP_HEADER_PERMANENT_HEADER_H
#define NET_HTTP_HEADER_PERMANENT_HEADER_H

#include <stdlib.h>
#include <stdint.h>
#include "net/http/header/type.h"
#include "string/buffer.h"
#include "string/slice.h"

namespace net {
  namespace http {
    namespace header {
      enum class permanent_field_name : uint8_t {
        kAccept,
        kAcceptCharset,
        kAcceptEncoding,
        kAcceptLanguage,
        kAcceptRanges,
        kAge,
        kAllow,
        kAuthorization,
        kCacheControl,
        kConnection,
        kContentEncoding,
        kContentLanguage,
        kContentLength,
        kContentLocation,
        kContentMD5,
        kContentRange,
        kContentType,
        kCookie,
        kDate,
        kETag,
        kExpect,
        kExpires,
        kFrom,
        kHost,
        kIfMatch,
        kIfModifiedSince,
        kIfNoneMatch,
        kIfRange,
        kIfUnmodifiedSince,
        kKeepAlive,
        kLastModified,
        kLocation,
        kMaxForwards,
        kPragma,
        kProxyAuthenticate,
        kProxyAuthorization,
        kProxyConnection,
        kRange,
        kReferer,
        kRetryAfter,
        kServer,
        kSetCookie,
        kStatus,
        kTE,
        kTrailer,
        kTransferEncoding,
        kUpgrade,
        kUserAgent,
        kVary,
        kVia,
        kWarning,
        kWWWAuthenticate,
        kUnknown = 0xff
      };

      class permanent_header {
        friend class permanent_headers;

        public:
          // Constructor.
          permanent_header();

          // Initialize.
          void init(permanent_field_name name,
                    size_t valueoff,
                    size_t valuelen);

          // Get name.
          static string::slice name(permanent_field_name name);
          string::slice name() const;

          // Get type.
          static type header_type(permanent_field_name name);
          type header_type() const;

          // Is list?
          static bool list(permanent_field_name name);
          bool list() const;

          // Single token?
          static bool single_token(permanent_field_name name);
          bool single_token() const;

          // Find permanent header field name.
          static permanent_field_name find(const char* s, size_t l);

        private:
          struct name {
            const char* name;
            size_t len;
          };

          static const struct name _M_names[];

          permanent_field_name _M_name;
          size_t _M_valueoff;
          size_t _M_valuelen;
      };

      class permanent_headers {
        public:
          static const size_t kMaxHeaders = 100;

          // Constructor.
          permanent_headers(const string::buffer& buf);

          // Destructor.
          ~permanent_headers();

          // Clear.
          void clear();

          // Reset.
          void reset();

          // Add permanent header.
          bool add(permanent_field_name name, size_t valueoff, size_t valuelen);

          // Remove permanent header.
          bool remove(permanent_field_name name);

          // Find permanent header.
          string::slice find(permanent_field_name name) const;

          // Get permanent header.
          bool get(size_t idx,
                   permanent_field_name& name,
                   string::slice& value) const;

        private:
          static const uint16_t kInitialAlloc = 4;

          const string::buffer& _M_buf;

          permanent_header* _M_headers;
          uint16_t _M_size;
          uint16_t _M_used;

          // Disable copy constructor and assignment operator.
          // Commented out: gcc bug 63707.
          //permanent_headers(const permanent_headers&) = delete;
          permanent_headers& operator=(const permanent_headers&) = delete;
      };

      inline permanent_header::permanent_header()
      {
      }

      inline void permanent_header::init(permanent_field_name name,
                                         size_t valueoff,
                                         size_t valuelen)
      {
        _M_name = name;
        _M_valueoff = valueoff;
        _M_valuelen = valuelen;
      }

      inline string::slice permanent_header::name(permanent_field_name name)
      {
        return string::slice(_M_names[static_cast<unsigned>(name)].name,
                             _M_names[static_cast<unsigned>(name)].len);
      }

      inline string::slice permanent_header::name() const
      {
        return string::slice(_M_names[static_cast<unsigned>(_M_name)].name,
                             _M_names[static_cast<unsigned>(_M_name)].len);
      }

      inline type permanent_header::header_type(permanent_field_name name)
      {
        static const net::http::header::type a[] = {
          /* kAccept             */ type::kRequest,
          /* kAcceptCharset      */ type::kRequest,
          /* kAcceptEncoding     */ type::kRequest,
          /* kAcceptLanguage     */ type::kRequest,
          /* kAcceptRanges       */ type::kResponse,
          /* kAge                */ type::kResponse,
          /* kAllow              */ type::kEntity,
          /* kAuthorization      */ type::kRequest,
          /* kCacheControl       */ type::kGeneral,
          /* kConnection         */ type::kGeneral,
          /* kContentEncoding    */ type::kEntity,
          /* kContentLanguage    */ type::kEntity,
          /* kContentLength      */ type::kEntity,
          /* kContentLocation    */ type::kEntity,
          /* kContentMD5         */ type::kEntity,
          /* kContentRange       */ type::kEntity,
          /* kContentType        */ type::kEntity,
          /* kCookie             */ type::kRequest,
          /* kDate               */ type::kGeneral,
          /* kETag               */ type::kResponse,
          /* kExpect             */ type::kRequest,
          /* kExpires            */ type::kEntity,
          /* kFrom               */ type::kRequest,
          /* kHost               */ type::kRequest,
          /* kIfMatch            */ type::kRequest,
          /* kIfModifiedSince    */ type::kRequest,
          /* kIfNoneMatch        */ type::kRequest,
          /* kIfRange            */ type::kRequest,
          /* kIfUnmodifiedSince  */ type::kRequest,
          /* kKeepAlive          */ type::kGeneral,
          /* kLastModified       */ type::kEntity,
          /* kLocation           */ type::kResponse,
          /* kMaxForwards        */ type::kRequest,
          /* kPragma             */ type::kGeneral,
          /* kProxyAuthenticate  */ type::kResponse,
          /* kProxyAuthorization */ type::kRequest,
          /* kProxyConnection    */ type::kGeneral,
          /* kRange              */ type::kRequest,
          /* kReferer            */ type::kRequest,
          /* kRetryAfter         */ type::kResponse,
          /* kServer             */ type::kResponse,
          /* kSetCookie          */ type::kResponse,
          /* kStatus             */ type::kResponse,
          /* kTE                 */ type::kRequest,
          /* kTrailer            */ type::kGeneral,
          /* kTransferEncoding   */ type::kGeneral,
          /* kUpgrade            */ type::kGeneral,
          /* kUserAgent          */ type::kRequest,
          /* kVary               */ type::kResponse,
          /* kVia                */ type::kGeneral,
          /* kWarning            */ type::kGeneral,
          /* kWWWAuthenticate    */ type::kResponse
        };

        return a[static_cast<unsigned>(name)];
      }

      inline type permanent_header::header_type() const
      {
        return type(_M_name);
      }

      inline bool permanent_header::list(permanent_field_name name)
      {
        static const bool a[] = {
          /* kAccept             */ true,
          /* kAcceptCharset      */ true,
          /* kAcceptEncoding     */ true,
          /* kAcceptLanguage     */ true,
          /* kAcceptRanges       */ true,
          /* kAge                */ false,
          /* kAllow              */ true,
          /* kAuthorization      */ false,
          /* kCacheControl       */ true,
          /* kConnection         */ true,
          /* kContentEncoding    */ true,
          /* kContentLanguage    */ true,
          /* kContentLength      */ false,
          /* kContentLocation    */ false,
          /* kContentMD5         */ false,
          /* kContentRange       */ false,
          /* kContentType        */ false,
          /* kCookie             */ true,
          /* kDate               */ false,
          /* kETag               */ false,
          /* kExpect             */ true,
          /* kExpires            */ false,
          /* kFrom               */ false,
          /* kHost               */ false,
          /* kIfMatch            */ true,
          /* kIfModifiedSince    */ false,
          /* kIfNoneMatch        */ true,
          /* kIfRange            */ false,
          /* kIfUnmodifiedSince  */ false,
          /* kKeepAlive          */ true,
          /* kLastModified       */ false,
          /* kLocation           */ false,
          /* kMaxForwards        */ false,
          /* kPragma             */ true,
          /* kProxyAuthenticate  */ true,
          /* kProxyAuthorization */ false,
          /* kProxyConnection    */ true,
          /* kRange              */ false,
          /* kReferer            */ false,
          /* kRetryAfter         */ false,
          /* kServer             */ true,
          /* kSetCookie          */ true,
          /* kStatus             */ false,
          /* kTE                 */ true,
          /* kTrailer            */ true,
          /* kTransferEncoding   */ true,
          /* kUpgrade            */ true,
          /* kUserAgent          */ true,
          /* kVary               */ true,
          /* kVia                */ true,
          /* kWarning            */ true,
          /* kWWWAuthenticate    */ true
        };

        return a[static_cast<unsigned>(name)];
      }

      inline bool permanent_header::list() const
      {
        return list(_M_name);
      }

      inline bool permanent_header::single_token(permanent_field_name name)
      {
        static const bool a[] = {
          /* kAccept             */ false,
          /* kAcceptCharset      */ false,
          /* kAcceptEncoding     */ false,
          /* kAcceptLanguage     */ false,
          /* kAcceptRanges       */ false,
          /* kAge                */ true,
          /* kAllow              */ false,
          /* kAuthorization      */ false,
          /* kCacheControl       */ false,
          /* kConnection         */ false,
          /* kContentEncoding    */ false,
          /* kContentLanguage    */ false,
          /* kContentLength      */ true,
          /* kContentLocation    */ true,
          /* kContentMD5         */ true,
          /* kContentRange       */ false,
          /* kContentType        */ false,
          /* kCookie             */ false,
          /* kDate               */ false,
          /* kETag               */ true,
          /* kExpect             */ false,
          /* kExpires            */ false,
          /* kFrom               */ true,
          /* kHost               */ true,
          /* kIfMatch            */ false,
          /* kIfModifiedSince    */ false,
          /* kIfNoneMatch        */ false,
          /* kIfRange            */ false,
          /* kIfUnmodifiedSince  */ false,
          /* kKeepAlive          */ false,
          /* kLastModified       */ false,
          /* kLocation           */ true,
          /* kMaxForwards        */ true,
          /* kPragma             */ false,
          /* kProxyAuthenticate  */ false,
          /* kProxyAuthorization */ false,
          /* kProxyConnection    */ false,
          /* kRange              */ false,
          /* kReferer            */ true,
          /* kRetryAfter         */ false,
          /* kServer             */ false,
          /* kSetCookie          */ false,
          /* kStatus             */ false,
          /* kTE                 */ false,
          /* kTrailer            */ false,
          /* kTransferEncoding   */ false,
          /* kUpgrade            */ false,
          /* kUserAgent          */ false,
          /* kVary               */ false,
          /* kVia                */ false,
          /* kWarning            */ false,
          /* kWWWAuthenticate    */ false
        };

        return a[static_cast<unsigned>(name)];
      }

      inline bool permanent_header::single_token() const
      {
        return single_token(_M_name);
      }

      inline permanent_headers::permanent_headers(const string::buffer& buf)
        : _M_buf(buf),
          _M_headers(NULL),
          _M_size(0),
          _M_used(0)
      {
      }

      inline permanent_headers::~permanent_headers()
      {
        if (_M_headers) {
          free(_M_headers);
        }
      }

      inline void permanent_headers::clear()
      {
        if (_M_headers) {
          free(_M_headers);
          _M_headers = NULL;
        }

        _M_size = 0;
        _M_used = 0;
      }

      inline void permanent_headers::reset()
      {
        _M_used = 0;
      }

      inline
      string::slice permanent_headers::find(permanent_field_name name) const
      {
        for (uint16_t i = 0; i < _M_used; i++) {
          const permanent_header* h = &_M_headers[i];
          if (name == h->_M_name) {
            return string::slice(_M_buf.data() + h->_M_valueoff, h->_M_valuelen);
          }
        }

        return string::slice();
      }

      inline bool permanent_headers::get(size_t idx,
                                         permanent_field_name& name,
                                         string::slice& value) const
      {
        if (idx < _M_used) {
          const permanent_header* h = &_M_headers[idx];

          name = h->_M_name;
          value.set(_M_buf.data() + h->_M_valueoff, h->_M_valuelen);

          return true;
        } else {
          return false;
        }
      }
    }
  }
}

#endif // NET_HTTP_HEADER_PERMANENT_HEADER_H
