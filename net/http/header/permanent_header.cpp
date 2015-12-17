#include <string.h>
#include "net/http/header/permanent_header.h"
#include "macros/macros.h"

const struct net::http::header::permanent_header::name
net::http::header::permanent_header::_M_names[] = {
  /* kAccept             */ {"Accept",               6},
  /* kAcceptCharset      */ {"Accept-Charset",      14},
  /* kAcceptEncoding     */ {"Accept-Encoding",     15},
  /* kAcceptLanguage     */ {"Accept-Language",     15},
  /* kAcceptRanges       */ {"Accept-Ranges",       13},
  /* kAge                */ {"Age",                  3},
  /* kAllow              */ {"Allow",                5},
  /* kAuthorization      */ {"Authorization",       13},
  /* kCacheControl       */ {"Cache-Control",       13},
  /* kConnection         */ {"Connection",          10},
  /* kContentEncoding    */ {"Content-Encoding",    16},
  /* kContentLanguage    */ {"Content-Language",    16},
  /* kContentLength      */ {"Content-Length",      14},
  /* kContentLocation    */ {"Content-Location",    16},
  /* kContentMD5         */ {"Content-MD5",         11},
  /* kContentRange       */ {"Content-Range",       13},
  /* kContentType        */ {"Content-Type",        12},
  /* kCookie             */ {"Cookie",               6},
  /* kDate               */ {"Date",                 4},
  /* kETag               */ {"ETag",                 4},
  /* kExpect             */ {"Expect",               6},
  /* kExpires            */ {"Expires",              7},
  /* kFrom               */ {"From",                 4},
  /* kHost               */ {"Host",                 4},
  /* kIfMatch            */ {"If-Match",             8},
  /* kIfModifiedSince    */ {"If-Modified-Since",   17},
  /* kIfNoneMatch        */ {"If-None-Match",       13},
  /* kIfRange            */ {"If-Range",             8},
  /* kIfUnmodifiedSince  */ {"If-Unmodified-Since", 19},
  /* kKeepAlive          */ {"Keep-Alive",          10},
  /* kLastModified       */ {"Last-Modified",       13},
  /* kLocation           */ {"Location",             8},
  /* kMaxForwards        */ {"Max-Forwards",        12},
  /* kPragma             */ {"Pragma",               6},
  /* kProxyAuthenticate  */ {"Proxy-Authenticate",  18},
  /* kProxyAuthorization */ {"Proxy-Authorization", 19},
  /* kProxyConnection    */ {"Proxy-Connection",    16},
  /* kRange              */ {"Range",                5},
  /* kReferer            */ {"Referer",              7},
  /* kRetryAfter         */ {"Retry-After",         11},
  /* kServer             */ {"Server",               6},
  /* kSetCookie          */ {"Set-Cookie",          10},
  /* kStatus             */ {"Status",               6},
  /* kTE                 */ {"TE",                   2},
  /* kTrailer            */ {"Trailer",              7},
  /* kTransferEncoding   */ {"Transfer-Encoding",   17},
  /* kUpgrade            */ {"Upgrade",              7},
  /* kUserAgent          */ {"User-Agent",          10},
  /* kVary               */ {"Vary",                 4},
  /* kVia                */ {"Via",                  3},
  /* kWarning            */ {"Warning",              7},
  /* kWWWAuthenticate    */ {"WWW-Authenticate",    16}
};

net::http::header::permanent_field_name
net::http::header::permanent_header::find(const char* s, size_t l)
{
  int i = 0;
  int j = ARRAY_SIZE(_M_names) - 1;

  while (i <= j) {
    int pivot = (i + j) / 2;
    int ret = strncasecmp(s, _M_names[pivot].name, l);
    if (ret < 0) {
      j = pivot - 1;
    } else if (ret == 0) {
      if (l < _M_names[pivot].len) {
        j = pivot - 1;
      } else if (l == _M_names[pivot].len) {
        return static_cast<permanent_field_name>(pivot);
      } else {
        i = pivot + 1;
      }
    } else {
      i = pivot + 1;
    }
  }

  return permanent_field_name::kUnknown;
}

bool net::http::header::permanent_headers::add(permanent_field_name name,
                                               size_t valueoff,
                                               size_t valuelen)
{
  // Too many headers?
  if (_M_used == kMaxHeaders) {
    return false;
  }

  if (_M_used == _M_size) {
    uint16_t size = (_M_size == 0) ? kInitialAlloc : (_M_size * 2);

    permanent_header* headers;
    if ((headers = reinterpret_cast<permanent_header*>(
                     realloc(_M_headers, size * sizeof(permanent_header))
                   )) == NULL) {
      return false;
    }

    _M_headers = headers;
    _M_size = size;
  }

  _M_headers[_M_used++].init(name, valueoff, valuelen);

  return true;
}

bool net::http::header::permanent_headers::remove(permanent_field_name name)
{
  for (uint16_t i = 0; i < _M_used; i++) {
    permanent_header* h = &_M_headers[i];
    if (name == h->_M_name) {
      if (i < --_M_used) {
        memmove(&_M_headers[i],
                &_M_headers[i + 1],
                (_M_used - i) * sizeof(permanent_header));
      }

      return true;
    }
  }

  return false;
}
