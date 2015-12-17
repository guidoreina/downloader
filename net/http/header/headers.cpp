#include <string.h>
#include "net/http/header/headers.h"
#include "util/ctype.h"
#include "net/http/header/ctype.h"
#include "constants/months_and_days.h"

bool net::http::header::headers::ranges(off_t filesize,
                                        util::ranges& ranges) const
{
  // If the header has not been added...
  if (!(_M_headers & (static_cast<uint64_t>(1) <<
                      static_cast<unsigned>(permanent_field_name::kRange)))) {
    return false;
  }

  string::slice s(_M_permanent_headers[static_cast<size_t>(
                                         permanent_header::header_type(
                                           permanent_field_name::kRange
                                         )
                                       )].find(permanent_field_name::kRange));

  // RFC 2616: 14.35 Range.

  if (s.length() < 8) {
    return false;
  }

  const char* v = s.data();
  if (strncasecmp(v, "bytes", 5) != 0) {
    return false;
  }

  const char* end = v + s.length();
  v += 5;

  off_t from = 0;
  off_t to = 0;
  off_t count = 0;

  int state = 0; // Waiting for '='.

  while (v < end) {
    uint8_t c = static_cast<uint8_t>(*v++);

    switch (state) {
      case 0: // Waiting for '='.
        if (c == '=') {
          state = 1; // After '='.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
      case 1: // After '='.
        if (util::is_digit(c)) {
          from = c - '0';

          state = 2; // Parsing from.
        } else if (c == '-') {
          state = 8; // Start of suffix-length.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
      case 2: // Parsing from.
        if (util::is_digit(c)) {
          off_t tmp;
          if ((tmp = (from * 10) + (c - '0')) < from) {
            return false;
          }

          from = tmp;
        } else if (c == '-') {
          state = 4; // After '-'.
        } else if (util::is_white_space(c)) {
          state = 3; // Whitespace after from.
        } else {
          return false;
        }

        break;
      case 3: // Whitespace after from.
        if (c == '-') {
          state = 4; // After '-'.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
      case 4: // After '-'.
        if (util::is_digit(c)) {
          to = c - '0';

          state = 5; // Parsing to.
        } else if (c == ',') {
          if (from < filesize) {
            if ((ranges.count() == kMaxRanges) ||
                (!ranges.add(from, filesize - 1))) {
              return false;
            }
          }

          state = 7; // After ','.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
      case 5: // Parsing to.
        if (util::is_digit(c)) {
          off_t tmp;
          if ((tmp = (to * 10) + (c - '0')) < to) {
            return false;
          }

          to = tmp;
        } else if (c == ',') {
          if (from > to) {
            return false;
          }

          if (from < filesize) {
            if ((ranges.count() == kMaxRanges) ||
                (!ranges.add(from, MIN(to, filesize - 1)))) {
              return false;
            }
          }

          state = 7; // After ','.
        } else if (util::is_white_space(c)) {
          if (from > to) {
            return false;
          }

          if (from < filesize) {
            if ((ranges.count() == kMaxRanges) ||
                (!ranges.add(from, MIN(to, filesize - 1)))) {
              return false;
            }
          }

          state = 6; // Whitespace after to.
        } else {
          return false;
        }

        break;
      case 6: // Whitespace after to.
        if (c == ',') {
          state = 7; // After ','.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
      case 7: // After ','.
        if (util::is_digit(c)) {
          from = c - '0';

          state = 2; // Parsing from.
        } else if (c == '-') {
          state = 8; // Start of suffix-length.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
      case 8: // Start of suffix-length.
        if (util::is_digit(c)) {
          count = c - '0';

          state = 9; // Parsing suffix-length.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
      case 9: // Parsing suffix-length.
        if (util::is_digit(c)) {
          off_t tmp;
          if ((tmp = (count * 10) + (c - '0')) < count) {
            return false;
          }

          count = tmp;
        } else if (c == ',') {
          if ((count > 0) && (filesize > 0)) {
            if ((ranges.count() == kMaxRanges) ||
                (!ranges.add(MAX(filesize - count, 0), filesize - 1))) {
              return false;
            }
          }

          state = 7; // After ','.
        } else if (util::is_white_space(c)) {
          if ((count > 0) && (filesize > 0)) {
            if ((ranges.count() == kMaxRanges) ||
                (!ranges.add(MAX(filesize - count, 0), filesize - 1))) {
              return false;
            }
          }

          state = 10; // Whitespace after suffix-length.
        } else {
          return false;
        }

        break;
      case 10: // Whitespace after suffix-length.
        if (c == ',') {
          state = 7; // After ','.
        } else if (!util::is_white_space(c)) {
          return false;
        }

        break;
    }
  }

  switch (state) {
    case 4:
      if (from >= filesize) {
        return true;
      }

      return ((ranges.count() < kMaxRanges) &&
              (ranges.add(from, filesize - 1)));
    case 5:
      if (from > to) {
        return false;
      }

      if (from >= filesize) {
        return true;
      }

      return ((ranges.count() < kMaxRanges) &&
              (ranges.add(from, MIN(to, filesize - 1))));
    case 6:
    case 10:
      return true;
    case 9:
      if ((count > 0) && (filesize > 0)) {
        if ((ranges.count() == kMaxRanges) ||
            (!ranges.add(MAX(filesize - count, 0), filesize - 1))) {
          return false;
        }
      }

      return true;
    default:
      return false;
  }
}

bool net::http::header::headers::add(permanent_field_name name,
                                     const char* value,
                                     size_t valuelen,
                                     bool clean_value)
{
  // If the header has been added already...
  if (_M_headers & (static_cast<uint64_t>(1) << static_cast<unsigned>(name))) {
    // If the header might appear only once...
    if (!permanent_header::list(name)) {
      return _M_ignore_errors;
    }
  }

  // Get current length of the buffer.
  size_t valueoff = _M_buf.length();

  // If the headers are too large...
  if (valueoff + valuelen > kHeadersMaxLen) {
    return false;
  }

  // If the value should be cleaned...
  if (clean_value) {
    if (!_M_buf.allocate(valuelen)) {
      return false;
    }

    int ret = _M_clean(value, valuelen, _M_buf.data() + valueoff);
    if (ret <= 0) {
      return _M_ignore_errors;
    }

    _M_buf.increment_length(ret);

    valuelen = ret;
  } else {
    if (!_M_buf.append(value, valuelen)) {
      return false;
    }
  }

  if (!_M_permanent_headers[static_cast<size_t>(
                              permanent_header::header_type(name)
                            )].add(name, valueoff, valuelen)) {
    return false;
  }

  _M_headers |= (static_cast<uint64_t>(1) << static_cast<unsigned>(name));

  return true;
}

bool net::http::header::headers::add(permanent_field_name name, int64_t n)
{
  // If the header has been added already...
  if (_M_headers & (static_cast<uint64_t>(1) << static_cast<unsigned>(name))) {
    // If the header might appear only once...
    if (!permanent_header::list(name)) {
      return _M_ignore_errors;
    }
  }

  // Get current length of the buffer.
  size_t valueoff = _M_buf.length();

  // If the headers are too large...
  if (valueoff >= kHeadersMaxLen) {
    return false;
  }

  if (!_M_buf.format("%lld", n)) {
    return false;
  }

  if (!_M_permanent_headers[static_cast<size_t>(
                              permanent_header::header_type(name)
                            )].add(name,
                                   valueoff,
                                   _M_buf.length() - valueoff)) {
    return false;
  }

  _M_headers |= (static_cast<uint64_t>(1) << static_cast<unsigned>(name));

  return true;
}

bool net::http::header::headers::add(permanent_field_name name, uint64_t n)
{
  // If the header has been added already...
  if (_M_headers & (static_cast<uint64_t>(1) << static_cast<unsigned>(name))) {
    // If the header might appear only once...
    if (!permanent_header::list(name)) {
      return _M_ignore_errors;
    }
  }

  // Get current length of the buffer.
  size_t valueoff = _M_buf.length();

  // If the headers are too large...
  if (valueoff >= kHeadersMaxLen) {
    return false;
  }

  if (!_M_buf.format("%llu", n)) {
    return false;
  }

  if (!_M_permanent_headers[static_cast<size_t>(
                              permanent_header::header_type(name)
                            )].add(name,
                                   valueoff,
                                   _M_buf.length() - valueoff)) {
    return false;
  }

  _M_headers |= (static_cast<uint64_t>(1) << static_cast<unsigned>(name));

  return true;
}

bool net::http::header::headers::add(const char* name,
                                     size_t namelen,
                                     const char* value,
                                     size_t valuelen,
                                     bool clean_value)
{
  // Get current length of the buffer.
  size_t valueoff = _M_buf.length();

  // If the headers are too large...
  if (valueoff + namelen + valuelen > kHeadersMaxLen) {
    return false;
  }

  // If the value should be cleaned...
  if (clean_value) {
    if (!_M_buf.allocate(valuelen)) {
      return false;
    }

    int ret = _M_clean(value, valuelen, _M_buf.data() + valueoff);
    if (ret <= 0) {
      return _M_ignore_errors;
    }

    _M_buf.increment_length(ret);

    valuelen = ret;
  } else {
    if (!_M_buf.append(value, valuelen)) {
      return false;
    }
  }

  // Get current length of the buffer.
  size_t nameoff = _M_buf.length();

  if (!_M_buf.append(name, namelen)) {
    return false;
  }

  return _M_non_permanent_headers.add(nameoff, namelen, valueoff, valuelen);
}

bool net::http::header::headers::add_timestamp(permanent_field_name name,
                                               time_t t)
{
  // If the header has been added already...
  if (_M_headers & (static_cast<uint64_t>(1) << static_cast<unsigned>(name))) {
    // If the header might appear only once...
    if (!permanent_header::list(name)) {
      return _M_ignore_errors;
    }
  }

  // If the headers are too large...
  if (_M_buf.length() >= kHeadersMaxLen) {
    return false;
  }

  struct tm tm;
  gmtime_r(&t, &tm);

  return _M_add_timestamp(name, tm);
}

bool net::http::header::headers::remove(permanent_field_name name)
{
  // If the header has not been added...
  if (!(_M_headers & (static_cast<uint64_t>(1) <<
                      static_cast<unsigned>(name)))) {
    return _M_ignore_errors;
  }

  _M_permanent_headers[static_cast<size_t>(
                         permanent_header::header_type(name)
                       )].remove(name);

  _M_headers &= ~(static_cast<uint64_t>(1) << static_cast<unsigned>(name));

  return true;
}

net::http::header::headers::parse_result
net::http::header::headers::parse(const void* buf, size_t len)
{
  const uint8_t* b = reinterpret_cast<const uint8_t*>(buf);
  const uint8_t* end = b + len;

  const uint8_t* ptr = b + _M_state.size;

  while (ptr < end) {
    uint8_t c = *ptr++;

    switch (_M_state.state) {
      case 0: // Initial state.
        switch (c) {
          case '\r':
            _M_state.state = 12; // After second '\r'.
            break;
          case '\n':
            _M_state.size++;
            return parse_result::kEndOfHeader;
          default:
            if (!header_name_valid_character(c)) {
              return parse_result::kInvalidHeader;
            }

            _M_state.name = _M_state.size;

            _M_state.state = 1; // Parsing header name.
        }

        break;
      case 1: // Parsing header name.
        if (c == ':') {
          _M_state.namelen = _M_state.size - _M_state.name;

          if ((_M_state.header = permanent_header::find(
                                   reinterpret_cast<const char*>(
                                     b + _M_state.name
                                   ),
                                   _M_state.namelen
                                 )) != permanent_field_name::kUnknown) {
            if (permanent_header::single_token(_M_state.header)) {
              _M_state.state = 2; // Single token - after colon.
            } else {
              _M_state.state = 8; // Multiple tokens - after colon.
            }
          } else {
            _M_state.state = 8; // Multiple tokens - after colon.
          }

          _M_state.value = 0;
        } else if (!header_name_valid_character(c)) {
          return parse_result::kInvalidHeader;
        }

        break;
      case 2: // Single token - after colon.
        switch (c) {
          case '\r':
            _M_state.state = 6; // Single token - after '\r'.
            break;
          case '\n':
            _M_state.state = 7; // Single token - after '\n'.
            break;
          default:
            if (header_value_valid_character(c)) {
              if (!util::is_white_space(c)) {
                _M_state.value = _M_state.size;

                _M_state.state = 3; // Single token - parsing header value.
              }
            } else {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 3: // Single token - parsing header value.
        switch (c) {
          case '\r':
            _M_state.value_end = _M_state.size;

            _M_state.state = 6; // Single token - after '\r'.
            break;
          case '\n':
            _M_state.value_end = _M_state.size;

            _M_state.state = 7; // Single token - after '\n'.
            break;
          default:
            if (header_value_valid_character(c)) {
              if (util::is_white_space(c)) {
                _M_state.value_end = _M_state.size;

                _M_state.state = 4; // Single token - after space.
              }
            } else {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 4: // Single token - after space.
        switch (c) {
          case '\r':
            _M_state.state = 6; // Single token - after '\r'.
            break;
          case '\n':
            _M_state.state = 7; // Single token - after '\n'.
            break;
          default:
            if (header_value_valid_character(c)) {
              if (!util::is_white_space(c)) {
                if (!_M_ignore_errors) {
                  return parse_result::kInvalidHeader;
                }

                _M_state.state = 5; // Single token - ignoring multiple tokens.
              }
            } else {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 5: // Single token - ignoring multiple tokens.
        switch (c) {
          case '\r':
            _M_state.state = 6; // Single token - after '\r'.
            break;
          case '\n':
            _M_state.state = 7; // Single token - after '\n'.
            break;
          default:
            if (!header_value_valid_character(c)) {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 6: // Single token - after '\r'.
        if (c != '\n') {
          return parse_result::kInvalidHeader;
        }

        _M_state.state = 7; // Single token - after '\n'.
        break;
      case 7: // Single token - after '\n'.
        switch (c) {
          case '\r':
            if (_M_state.value != 0) {
              if (!add(_M_state.header,
                       reinterpret_cast<const char*>(b + _M_state.value),
                       _M_state.value_end - _M_state.value)) {
                return parse_result::kInvalidHeader;
              }
            }

            _M_state.state = 12; // After second '\r'.
            break;
          case '\n':
            if (_M_state.value != 0) {
              if (!add(_M_state.header,
                       reinterpret_cast<const char*>(b + _M_state.value),
                       _M_state.value_end - _M_state.value)) {
                return parse_result::kInvalidHeader;
              }
            }

            _M_state.size++;

            return parse_result::kEndOfHeader;
          default:
            if (header_name_valid_character(c)) {
              if (_M_state.value != 0) {
                if (!add(_M_state.header,
                         reinterpret_cast<const char*>(b + _M_state.value),
                         _M_state.value_end - _M_state.value)) {
                  return parse_result::kInvalidHeader;
                }
              }

              _M_state.name = _M_state.size;

              _M_state.state = 1; // Parsing header name.
            } else if (util::is_white_space(c)) {
              // Folded header value.
              _M_state.state = (_M_state.value == 0) ? 2 : 4;
            } else {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 8: // Multiple tokens - after colon.
        switch (c) {
          case '\r':
            _M_state.state = 10; // Multiple tokens - after '\r'.
            break;
          case '\n':
            _M_state.state = 11; // Multiple tokens - after '\n'.
            break;
          default:
            if (header_value_valid_character(c)) {
              if (!util::is_white_space(c)) {
                _M_state.value = _M_state.size;
                _M_state.value_end = 0;

                _M_state.state = 9; // Multiple tokens - parsing header value.
              }
            } else {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 9: // Multiple tokens - parsing header value.
        switch (c) {
          case '\r':
            if (_M_state.value_end == 0) {
              _M_state.value_end = _M_state.size;
            }

            _M_state.state = 10; // Multiple tokens - after '\r'.
            break;
          case '\n':
            if (_M_state.value_end == 0) {
              _M_state.value_end = _M_state.size;
            }

            _M_state.state = 11; // Multiple tokens - after '\n'.
            break;
          default:
            if (header_value_valid_character(c)) {
              if (util::is_white_space(c)) {
                if (_M_state.value_end == 0) {
                  _M_state.value_end = _M_state.size;
                }
              } else {
                _M_state.value_end = 0;
              }
            } else {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 10: // Multiple tokens - after '\r'.
        if (c != '\n') {
          return parse_result::kInvalidHeader;
        }

        _M_state.state = 11; // Multiple tokens - after '\n'.
        break;
      case 11: // Multiple tokens - after '\n'.
        switch (c) {
          case '\r':
            if (_M_state.value != 0) {
              if (_M_state.header != permanent_field_name::kUnknown) {
                if (!add(_M_state.header,
                         reinterpret_cast<const char*>(b + _M_state.value),
                         _M_state.value_end - _M_state.value,
                         true /* Clean value. */)) {
                  return parse_result::kInvalidHeader;
                }
              } else {
                if (!add(reinterpret_cast<const char*>(b + _M_state.name),
                         _M_state.namelen,
                         reinterpret_cast<const char*>(b + _M_state.value),
                         _M_state.value_end - _M_state.value,
                         true /* Clean value. */)) {
                  return parse_result::kInvalidHeader;
                }
              }
            }

            _M_state.state = 12; // After second '\r'.
            break;
          case '\n':
            if (_M_state.value != 0) {
              if (_M_state.header != permanent_field_name::kUnknown) {
                if (!add(_M_state.header,
                         reinterpret_cast<const char*>(b + _M_state.value),
                         _M_state.value_end - _M_state.value,
                         true /* Clean value. */)) {
                  return parse_result::kInvalidHeader;
                }
              } else {
                if (!add(reinterpret_cast<const char*>(b + _M_state.name),
                         _M_state.namelen,
                         reinterpret_cast<const char*>(b + _M_state.value),
                         _M_state.value_end - _M_state.value,
                         true /* Clean value. */)) {
                  return parse_result::kInvalidHeader;
                }
              }
            }

            _M_state.size++;

            return parse_result::kEndOfHeader;
          default:
            if (header_name_valid_character(c)) {
              if (_M_state.value != 0) {
                if (_M_state.header != permanent_field_name::kUnknown) {
                  if (!add(_M_state.header,
                           reinterpret_cast<const char*>(b + _M_state.value),
                           _M_state.value_end - _M_state.value,
                           true /* Clean value. */)) {
                    return parse_result::kInvalidHeader;
                  }
                } else {
                  if (!add(reinterpret_cast<const char*>(b + _M_state.name),
                           _M_state.namelen,
                           reinterpret_cast<const char*>(b + _M_state.value),
                           _M_state.value_end - _M_state.value,
                           true /* Clean value. */)) {
                    return parse_result::kInvalidHeader;
                  }
                }
              }

              _M_state.name = _M_state.size;

              _M_state.state = 1; // Parsing header name.
            } else if (util::is_white_space(c)) {
              // Folded header value.
              _M_state.state = (_M_state.value == 0) ? 8 : 9;
            } else {
              return parse_result::kInvalidHeader;
            }
        }

        break;
      case 12: // After second '\r'.
        if (c != '\n') {
          return parse_result::kInvalidHeader;
        }

        _M_state.size++;

        return parse_result::kEndOfHeader;
    }

    if (++_M_state.size == kHeadersMaxLen) {
      return parse_result::kHeadersTooLarge;
    }
  }

  return parse_result::kNotEndOfHeader;
}

bool net::http::header::headers::serialize(string::buffer& buf) const
{
  string::slice v;

  for (size_t i = 0; i < ARRAY_SIZE(_M_permanent_headers); i++) {
    permanent_field_name name;
    for (size_t j = 0; _M_permanent_headers[i].get(j, name, v); j++) {
      string::slice n = permanent_header::name(name);

      if ((!buf.append(n.data(), n.length())) ||
          (!buf.append(": ", 2)) ||
          (!buf.append(v.data(), v.length())) ||
          (!buf.append("\r\n", 2))) {
        return false;
      }
    }
  }

  string::slice n;
  for (size_t j = 0; _M_non_permanent_headers.get(j, n, v); j++) {
    if ((!buf.append(n.data(), n.length())) ||
        (!buf.append(": ", 2)) ||
        (!buf.append(v.data(), v.length())) ||
        (!buf.append("\r\n", 2))) {
      return false;
    }
  }

  return buf.append("\r\n", 2);
}

bool net::http::header::headers::_M_add_timestamp(permanent_field_name name,
                                                  const struct tm& tm)
{
  // Get current length of the buffer.
  size_t valueoff = _M_buf.length();

  if (!_M_buf.format("%s, %02u %s %u %02u:%02u:%02u GMT",
                     constants::days[tm.tm_wday],
                     tm.tm_mday,
                     constants::months[tm.tm_mon],
                     1900 + tm.tm_year,
                     tm.tm_hour,
                     tm.tm_min,
                     tm.tm_sec)) {
    return false;
  }

  if (!_M_permanent_headers[static_cast<size_t>(
                              permanent_header::header_type(name)
                            )].add(name,
                                   valueoff,
                                   _M_buf.length() - valueoff)) {
    return false;
  }

  _M_headers |= (static_cast<uint64_t>(1) << static_cast<unsigned>(name));

  return true;
}

int net::http::header::headers::_M_clean(const char* value,
                                         size_t valuelen,
                                         char* dest)
{
  const char* ptr = value;
  const char* end = ptr + valuelen;
  char* d = dest;

  int state = 0;

  while (ptr < end) {
    uint8_t c = static_cast<uint8_t>(*ptr++);

    switch (state) {
      case 0:
        switch (c) {
          case '\r':
            state = 2; // After '\r'.
            break;
          case '\n':
            state = 3; // After '\n'.
            break;
          default:
            if (header_value_valid_character(c)) {
              if (util::is_white_space(c)) {
                state = 1; // After space.
              } else {
                *d++ = c;
              }
            } else {
              return -1;
            }
        }

        break;
      case 1:
        switch (c) {
          case '\r':
            state = 2; // After '\r'.
            break;
          case '\n':
            state = 3; // After '\n'.
            break;
          default:
            if (header_value_valid_character(c)) {
              if (!util::is_white_space(c)) {
                // If not the first character.
                if (d > dest) {
                  *d++ = ' ';
                }

                *d++ = c;

                state = 0;
              }
            } else {
              return -1;
            }
        }

        break;
      case 2:
        // After '\r'.
        if (c != '\n') {
          return -1;
        }

        state = 3; // After '\n'.
        break;
      default:
        // After '\n'.
        if (!util::is_white_space(c)) {
          return -1;
        }

        state = 1; // After space.
    }
  }

  return d - dest;
}
