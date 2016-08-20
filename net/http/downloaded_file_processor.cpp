#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "net/http/downloaded_file_processor.h"
#include "string/memcasemem.h"
#include "util/number.h"
#include "util/ctype.h"
#include "net/uri/ctype.h"

bool net::http::downloaded_file_processor::open(const char* filename)
{
  struct stat sbuf;
  if ((stat(filename, &sbuf) < 0) || (!S_ISREG(sbuf.st_mode))) {
    return false;
  }

  if ((_M_fd = ::open(filename, O_RDONLY)) < 0) {
    return false;
  }

  void* data;
  if ((data = mmap(NULL,
                   sbuf.st_size,
                   PROT_READ,
                   MAP_SHARED,
                   _M_fd,
                   0)) == MAP_FAILED) {
    return false;
  }

  _M_len = sbuf.st_size;

  _M_data = reinterpret_cast<uint8_t*>(data);
  _M_end = _M_data + _M_len;

  _M_ptr = _M_data;

  _M_content_type = content_type::kOther;

  return ((read_status_code()) && (read_headers()));
}

bool net::http::downloaded_file_processor::read_title(string::slice& title)
{
  // If not text/html...
  if (_M_content_type != content_type::kTextHtml) {
    return false;
  }

  const uint8_t* t;
  if ((t = reinterpret_cast<const uint8_t*>(string::memcasemem(_M_body,
                                                               _M_end - _M_body,
                                                               "<title",
                                                               6))) == NULL) {
    return false;
  }

  t += 6;

  while ((t < _M_end) && (*t != '>')) {
    t++;
  }

  if (t == _M_end) {
    return false;
  }

  // Skip '>'.
  t++;

  // Remove spaces before the title.
  while ((t < _M_end) && (*t <= ' ')) {
    t++;
  }

  if (t == _M_end) {
    return false;
  }

  const uint8_t* end = t;
  const uint8_t* last = NULL;

  while ((end < _M_end) && (*end != '<')) {
    if (*end > ' ') {
      last = end;
    }

    end++;
  }

  if (last) {
    title.set(reinterpret_cast<const char*>(t), last - t + 1);
  }

  return true;
}

bool net::http::downloaded_file_processor::next(uri::uri& uri)
{
  // If this is the first time this method is called...
  if (_M_ptr == _M_body) {
    if (_M_config.urls_begin) {
      size_t len = strlen(_M_config.urls_begin);

      if ((_M_ptr = reinterpret_cast<const uint8_t*>(
                      string::memcasemem(_M_ptr,
                                         _M_end - _M_ptr,
                                         _M_config.urls_begin,
                                         len)
                    )) == NULL) {
        return false;
      }

      _M_ptr += len;
    }

    if (_M_config.urls_end) {
      const uint8_t* end;
      if ((end = reinterpret_cast<const uint8_t*>(
                   string::memcasemem(_M_ptr,
                                      _M_end - _M_ptr,
                                      _M_config.urls_end,
                                      strlen(_M_config.urls_end))
                 )) != NULL) {
        _M_end = end;
      }
    }
  }

  _M_buf.clear();

  if (!_M_buf.allocate(2 * 1024)) {
    return false;
  }

  do {
    // Find next URI.
    if ((_M_ptr = reinterpret_cast<const uint8_t*>(
                    string::memcasemem(_M_ptr,
                                       _M_end - _M_ptr,
                                       "http",
                                       4)
                  )) == NULL) {
      return false;
    }

    // If the URI is beyond the area where to search for URIs...
    if (_M_ptr >= _M_end) {
      return false;
    }

    // If the URI is too small...
    if (_M_end - _M_ptr < 8) {
      return false;
    }

    const uint8_t* pos = _M_ptr + 4;
    int state = 0; // Waiting for 's' or ':'.

    const uint8_t* host = NULL;
    const uint8_t* path = NULL;

    while ((pos < _M_end) &&
           ((state >= 0) && (state <= 7)) &&
           (!strchr(_M_config.url_separators, *pos))) {
      uint8_t c = *pos;
      if (c == '%') {
        if (pos + 2 >= _M_end) {
          return false;
        }

        int n;
        if ((n = util::hex2dec(*++pos)) < 0) {
          break;
        }

        c = n * 16;

        if ((n = util::hex2dec(*++pos)) < 0) {
          break;
        }

        c += n;
      }

      switch (state) {
        case 0: // Waiting for 's' or ':'.
          switch (c) {
            case 's':
            case 'S':
              _M_buf.clear();

              _M_buf.append("https", 5);

              state = 1; // Waiting for ':'.
              break;
            case ':':
              _M_buf.clear();

              _M_buf.append("http", 4);

              state = 2; // Waiting for first '/'.
              break;
            default:
              state = -1; // Error.
          }

          break;
        case 1: // Waiting for ':'.
          if (c == ':') {
            state = 2; // Waiting for first '/'.
          } else {
            state = -1; // Error.
          }

          break;
        case 2: // Waiting for first '/'.
          if (c == '/') {
            state = 3; // Waiting for second '/'.
          } else {
            state = -1; // Error.
          }

          break;
        case 3: // Waiting for second '/'.
          if (c == '/') {
            state = 4; // Start of host.
          } else {
            state = -1; // Error.
          }

          break;
        case 4: // Start of host.
          _M_buf.append("://", 3);

          host = pos;

          state = 5; // Parsing host.
          break;
        case 5: // Parsing host.
          if (c == '/') {
            size_t len = pos - host;

            // If the '/' was encoded...
            if (*pos != c) {
              len -= 2;
            }

            if ((!_M_buf.append(reinterpret_cast<const char*>(host), len)) ||
                (!_M_buf.append('/'))) {
              return false;
            }

            path = pos + 1;

            state = 6; // Parsing path.
          }

          break;
        case 6: // Parsing path.
          if (c == '?') {
            size_t len = pos - path;

            // If the '?' was encoded...
            if (*pos != c) {
              len -= 2;
            }

            if ((!_M_buf.append(reinterpret_cast<const char*>(path), len)) ||
                (!_M_buf.append('?'))) {
              return false;
            }

            state = 7; // Parsing query.
          }

          break;
        case 7: // Parsing query.
          if ((c == '\\') &&
              (pos + 5 < _M_end) &&
              (pos[1] == 'u') &&
              (util::is_xdigit(pos[2])) &&
              (util::is_xdigit(pos[3])) &&
              (util::is_xdigit(pos[4])) &&
              (util::is_xdigit(pos[5]))) {
            uint8_t ch = (util::hex2dec(pos[2]) * 16) + util::hex2dec(pos[3]);

            if (ch != 0) {
              if (!_M_buf.append(ch)) {
                return false;
              }
            }

            ch = (util::hex2dec(pos[4]) * 16) + util::hex2dec(pos[5]);

            if (ch != 0) {
              if (!_M_buf.append(ch)) {
                return false;
              }
            }

            pos += 5;
          } else if (c == '#') {
            state = 8; // End.
          } else {
            if ((uri::is_valid_query_or_fragment_char(&c, &c)) || (c == '%')) {
              if (!_M_buf.append(c)) {
                return false;
              }
            } else {
              if (!_M_buf.format("%%%02x", c)) {
                return false;
              }
            }
          }

          break;
      }

      pos++;
    }

    switch (state) {
      case 5: // Parsing host.
        if ((!_M_buf.append(reinterpret_cast<const char*>(host), pos - host)) ||
            (!_M_buf.append('/'))) {
          return false;
        }

        break;
      case 6: // Parsing path.
        if (!_M_buf.append(reinterpret_cast<const char*>(path), pos - path)) {
          return false;
        }

        break;
      case 7: // Parsing query.
        break;
      case 8: // End.
        break;
      default:
        state = -1; // Error.
    }

    _M_ptr = pos;

    if (state != -1) {
      uri.clear();

      if (uri.init(_M_buf.data(), _M_buf.length())) {
        return true;
      }
    }
  } while (true);
}

bool net::http::downloaded_file_processor::read_status_code()
{
  // Skip first line (URI).
  if ((_M_ptr = reinterpret_cast<const uint8_t*>(
                  memmem(_M_data, _M_len, "\r\n", 2)
                )) == NULL) {
    return false;
  }

  // Save URI.
  _M_uri.set(reinterpret_cast<const char*>(_M_data), _M_ptr - _M_data);

  _M_ptr += 2;

  // Skip HTTP version.
  while ((_M_ptr < _M_end) && (*_M_ptr > ' ')) {
    _M_ptr++;
  }

  if (_M_ptr == _M_end) {
    return false;
  }

  // Skip white spaces.
  while ((++_M_ptr < _M_end) && (util::is_white_space(*_M_ptr))) {
    _M_ptr++;
  }

  if (_M_ptr == _M_end) {
    return false;
  }

  // Parse status-code.
  if (_M_ptr + 3 >= _M_end) {
    return false;
  }

  if (util::number::parse(_M_ptr, 3, _M_status_code, 100, 999) !=
      util::number::parse_result::kSucceeded) {
    return false;
  }

  // Point '_M_ptr' to the headers.
  if ((_M_ptr = reinterpret_cast<const uint8_t*>(
                  memmem(_M_ptr,
                         _M_len - (_M_ptr - _M_data),
                         "\r\n",
                         2)
                )) == NULL) {
    return false;
  }

  _M_ptr += 2;

  return true;
}

bool net::http::downloaded_file_processor::read_headers()
{
  if (_M_headers.parse(_M_ptr, _M_end - _M_ptr) !=
      header::headers::parse_result::kEndOfHeader) {
    return false;
  }

  // Get Content-Type.
  string::slice
        type(_M_headers.header(header::permanent_field_name::kContentType));

  if (type.length() >= 9) {
    const char* ptr;
    if ((ptr = reinterpret_cast<const char*>(
                 string::memcasemem(type.data(),
                                    type.length(),
                                    "text/",
                                    5)
               )) != NULL) {
      const char* end = type.data() + type.length();
      ptr += 5;

      size_t left;
      if ((left = end - ptr) >= 4) {
        if (strncasecmp(ptr, "html", 4) == 0) {
          _M_content_type = content_type::kTextHtml;
        } else if (left >= 5) {
          if (strncasecmp(ptr, "plain", 5) == 0) {
            _M_content_type = content_type::kTextPlain;
          }
        }
      }
    }
  }

  _M_ptr += _M_headers.size();
  _M_body = _M_ptr;

  return true;
}
