#include "net/http/header/non_permanent_header.h"

bool net::http::header::non_permanent_headers::add(size_t nameoff,
                                                   size_t namelen,
                                                   size_t valueoff,
                                                   size_t valuelen)
{
  // Too many headers?
  if (_M_used == kMaxHeaders) {
    return false;
  }

  if (_M_used == _M_size) {
    uint16_t size = (_M_size == 0) ? kInitialAlloc : (_M_size * 2);

    non_permanent_header* headers;
    if ((headers = reinterpret_cast<non_permanent_header*>(
                     realloc(_M_headers, size * sizeof(non_permanent_header))
                   )) == NULL) {
      return false;
    }

    _M_headers = headers;
    _M_size = size;
  }

  _M_headers[_M_used++].init(nameoff, namelen, valueoff, valuelen);

  return true;
}

bool net::http::header::non_permanent_headers::remove(const char* s, size_t len)
{
  for (uint16_t i = 0; i < _M_used; i++) {
    non_permanent_header* h = &_M_headers[i];
    if ((len == h->_M_namelen) &&
        (strncasecmp(s, _M_buf.data() + h->_M_nameoff, len) == 0)) {
      if (i < --_M_used) {
        memmove(&_M_headers[i],
                &_M_headers[i + 1],
                (_M_used - i) * sizeof(non_permanent_header));
      }

      return true;
    }
  }

  return false;
}
