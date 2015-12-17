#include <stdlib.h>
#include <stdio.h>
#include "net/uri/uri.h"
#include "net/uri/ctype.h"
#include "net/ports.h"
#include "util/number.h"

bool net::uri::uri::init(const void* s, size_t n)
{
  // Initialize parser.
  if (!_M_parser.init(s, n)) {
    return false;
  }

  // Parse scheme.
  if (!_M_parser.parse_scheme()) {
    return false;
  }

  // Current character is ':'.

  // Move to the next character.
  if (!_M_parser.advance()) {
    // path-empty
    return init_from_parser();
  }

  // Save possible start of the path.
  _M_parser.path = _M_parser.p;

  if (*_M_parser.p == '/') {
    // Move to the next character.
    if (!_M_parser.advance()) {
      // path-absolute
      _M_parser.pathlen = 1;
      return init_from_parser();
    }

    if (*_M_parser.p == '/') {
      // "//" authority path-abempty

      // Move to the next character.
      if (!_M_parser.advance()) {
        return false;
      }

      // Parse authority.
      if (!_M_parser.parse_authority()) {
        return false;
      }

      // End?
      if (_M_parser.p == _M_parser.end) {
        return init_from_parser();
      }

      // Path?
      if (*_M_parser.p == '/') {
        // Save start of the path.
        _M_parser.path = _M_parser.p++;

        if (!_M_parser.parse_path()) {
          return false;
        }
      }
    } else {
      // path-absolute
      if (!_M_parser.parse_path()) {
        return false;
      }
    }
  } else {
    // path-rootless
    if (!_M_parser.parse_path()) {
      return false;
    }
  }

  // End?
  if (_M_parser.p == _M_parser.end) {
    return init_from_parser();
  }

  // Query?
  if (*_M_parser.p == '?') {
    // Save start of the query.
    _M_parser.query = ++_M_parser.p;

    _M_parser.parse_query();

    // End?
    if (_M_parser.p == _M_parser.end) {
      return init_from_parser();
    }
  }

  // Fragment?
  if (*_M_parser.p == '#') {
    // Save start of the fragment.
    _M_parser.fragment = ++_M_parser.p;

    _M_parser.parse_fragment();

    // End?
    if (_M_parser.p == _M_parser.end) {
      return init_from_parser();
    }
  }

  return false;
}

bool net::uri::uri::init(const uri& other)
{
  if (!_M_uri.append(other._M_uri.data(), other._M_uri.length())) {
    return false;
  }

  const char* d = _M_uri.data();
  const char* otherd = other._M_uri.data();

  _M_scheme.set(d, other._M_scheme.length());

  if (other._M_hier_part.userinfo.length() > 0) {
    _M_hier_part.userinfo.set(d + (other._M_hier_part.userinfo.data() - otherd),
                              other._M_hier_part.userinfo.length());
  }

  _M_hier_part.ip_literal = other._M_hier_part.ip_literal;

  if (other._M_hier_part.host.length() > 0) {
    _M_hier_part.host.set(d + (other._M_hier_part.host.data() - otherd),
                          other._M_hier_part.host.length());
  }

  _M_hier_part.port = other._M_hier_part.port;

  if (other._M_hier_part.path.length() > 0) {
    _M_hier_part.path.set(d + (other._M_hier_part.path.data() - otherd),
                          other._M_hier_part.path.length());
  }

  if (other._M_query.length() > 0) {
    _M_query.set(d + (other._M_query.data() - otherd), other._M_query.length());
  }

  if (other._M_fragment.length() > 0) {
    _M_fragment.set(d + (other._M_fragment.data() - otherd),
                    other._M_fragment.length());
  }

  return true;
}

bool net::uri::uri::normalize(uri& other) const
{
  if (!other._M_uri.allocate(_M_uri.length() + 1)) {
    return false;
  }

  const char* d = _M_uri.data();
  const char* end = d + _M_uri.length();

  char* otherd = other._M_uri.data();

  // Copy scheme.
  size_t len = _M_scheme.length();
  for (size_t i = 0; i < len; i++) {
    *otherd++ = util::to_lower(*d++);
  }

  other._M_scheme.set(other._M_uri.data(), len);

  // Skip colon.
  d++;

  *otherd++ = ':';

  // If there is authority...
  if (_M_hier_part.host.length() > 0) {
    d += 2;

    *otherd++ = '/';
    *otherd++ = '/';

    // If there is user information...
    if ((len = _M_hier_part.userinfo.length()) > 0) {
      // Save start of the userinfo.
      char* userinfo = otherd;

      size_t i = 0;
      do {
        if (*d == '%') {
          uint8_t c = (util::hex2dec(d[1]) * 16) + util::hex2dec(d[2]);
          if (is_unreserved(c)) {
            *otherd++ = static_cast<char>(c);
          } else {
            *otherd++ = '%';
            *otherd++ = util::to_upper(d[1]);
            *otherd++ = util::to_upper(d[2]);
          }

          d += 3;
          i += 2;
        } else {
          *otherd++ = *d++;
        }
      } while (++i < len);

      other._M_hier_part.userinfo.set(userinfo, otherd - userinfo);

      d++;
      *otherd++ = '@';
    }

    // IP literal?
    if (_M_hier_part.ip_literal) {
      // Skip '['.
      d++;

      *otherd++ = '[';
    }

    // Save start of the host.
    char* host = otherd;

    // Copy host.
    len = _M_hier_part.host.length();
    size_t i = 0;
    do {
      if (*d == '%') {
        uint8_t c = (util::hex2dec(d[1]) * 16) + util::hex2dec(d[2]);
        if (is_unreserved(c)) {
          *otherd++ = static_cast<char>(util::to_lower(c));
        } else {
          *otherd++ = '%';
          *otherd++ = util::to_upper(d[1]);
          *otherd++ = util::to_upper(d[2]);
        }

        d += 3;
        i += 2;
      } else {
        *otherd++ = util::to_lower(*d++);
      }
    } while (++i < len);

    other._M_hier_part.host.set(host, otherd - host);

    // IP literal?
    if (_M_hier_part.ip_literal) {
      // Skip ']'.
      d++;

      *otherd++ = ']';
    }

    other._M_hier_part.ip_literal = _M_hier_part.ip_literal;

    // If not a standard port...
    if (_M_hier_part.port != 0) {
      *otherd++ = ':';
      otherd += sprintf(otherd, "%u", _M_hier_part.port);
    }

    other._M_hier_part.port = _M_hier_part.port;
  }

  // Save start of the path.
  char* path = otherd;

  // If the path is empty...
  if ((len = _M_hier_part.path.length()) == 0) {
    *otherd++ = '/';
  } else {
    // Remove dot segments.
    d = _M_hier_part.path.data();

    size_t i = 0;
    do {
      if (*d == '%') {
        uint8_t c = (util::hex2dec(d[1]) * 16) + util::hex2dec(d[2]);
        if (is_unreserved(c)) {
          *otherd++ = static_cast<char>(c);
        } else {
          *otherd++ = '%';
          *otherd++ = util::to_upper(d[1]);
          *otherd++ = util::to_upper(d[2]);
        }

        d += 3;
        i += 2;
      } else {
        *otherd++ = *d++;
      }

      // A. If the input buffer begins with a prefix of "../" or "./",
      //    then remove that prefix from the input buffer; otherwise,
      if (((path + 3 == otherd) &&
           (path[0] == '.') &&
           (path[1] == '.') &&
           (path[2] == '/')) ||
          ((path + 2 == otherd) &&
           (path[0] == '.') &&
           (path[1] == '/'))) {
        otherd = path;

      // B. if the input buffer begins with a prefix of "/./" or "/.",
      //    where "." is a complete path segment, then replace that
      //    prefix with "/" in the input buffer; otherwise,
      } else if ((path + 3 <= otherd) &&
                 (otherd[-3] == '/') &&
                 (otherd[-2] == '.') &&
                 (otherd[-1] == '/')) {
        otherd -= 2;
      } else if ((path + 2 <= otherd) &&
                 (d == end) &&
                 (otherd[-2] == '/') &&
                 (otherd[-1] == '.')) {
        otherd--;

      // C. if the input buffer begins with a prefix of "/../" or "/..",
      //    where ".." is a complete path segment, then replace that
      //    prefix with "/" in the input buffer and remove the last
      //    segment and its preceding "/" (if any) from the output
      //    buffer; otherwise,
      } else if ((path + 4 <= otherd) &&
                 (otherd[-4] == '/') &&
                 (otherd[-3] == '.') &&
                 (otherd[-2] == '.') &&
                 (otherd[-1] == '/')) {
        otherd -= 4;
        while (otherd > path) {
          if (*--otherd == '/') {
            break;
          }
        }

        *otherd++ = '/';
      } else if ((path + 3 <= otherd) &&
                 (d == end) &&
                 (otherd[-3] == '/') &&
                 (otherd[-2] == '.') &&
                 (otherd[-1] == '.')) {
        otherd -= 3;
        while (otherd > path) {
          if (*--otherd == '/') {
            break;
          }
        }

        *otherd++ = '/';

      // D. if the input buffer consists only of "." or "..", then remove
      // that from the input buffer; otherwise,
      } else if (((path + 1 == otherd) &&
                  (d == end) &&
                  (otherd[-1] == '.')) ||
                 ((path + 2 == otherd) &&
                  (d == end) &&
                  (otherd[-2] == '.') &&
                  (otherd[-1] == '.'))) {
        otherd = path;
      }
    } while (++i < len);
  }

  if (path < otherd) {
    other._M_hier_part.path.set(path, otherd - path);
  }

  // If there is query...
  if ((len = _M_query.length()) > 0) {
    *otherd++ = '?';

    // Save start of the query.
    char* query = otherd;

    d = _M_query.data();

    size_t i = 0;
    do {
      if (*d == '%') {
        uint8_t c = (util::hex2dec(d[1]) * 16) + util::hex2dec(d[2]);
        if (is_unreserved(c)) {
          *otherd++ = static_cast<char>(c);
        } else {
          *otherd++ = '%';
          *otherd++ = util::to_upper(d[1]);
          *otherd++ = util::to_upper(d[2]);
        }

        d += 3;
        i += 2;
      } else {
        *otherd++ = *d++;
      }
    } while (++i < len);

    other._M_query.set(query, otherd - query);
  }

  // If there is fragment...
  if ((len = _M_fragment.length()) > 0) {
    *otherd++ = '#';

    // Save start of the fragment.
    char* fragment = otherd;

    d = _M_fragment.data();

    size_t i = 0;
    do {
      if (*d == '%') {
        uint8_t c = (util::hex2dec(d[1]) * 16) + util::hex2dec(d[2]);
        if (is_unreserved(c)) {
          *otherd++ = static_cast<char>(c);
        } else {
          *otherd++ = '%';
          *otherd++ = util::to_upper(d[1]);
          *otherd++ = util::to_upper(d[2]);
        }

        d += 3;
        i += 2;
      } else {
        *otherd++ = *d++;
      }
    } while (++i < len);

    other._M_fragment.set(fragment, otherd - fragment);
  }

  other._M_uri.length(otherd - other._M_uri.data());

  return true;
}

bool net::uri::uri::init_from_parser()
{
  // Save original URI.
  if (!_M_uri.append(reinterpret_cast<const char*>(_M_parser.begin),
                     _M_parser.end - _M_parser.begin)) {
    return false;
  }

  const char* d = _M_uri.data();

  _M_scheme.set(d, _M_parser.schemelen);

  if (_M_parser.userinfolen > 0) {
    _M_hier_part.userinfo.set(d + (_M_parser.userinfo - _M_parser.begin),
                              _M_parser.userinfolen);
  }

  _M_hier_part.ip_literal = _M_parser.ip_literal;

  if (_M_parser.hostlen > 0) {
    _M_hier_part.host.set(d + (_M_parser.host - _M_parser.begin),
                          _M_parser.hostlen);
  }

  _M_hier_part.port = _M_parser.port;

  if (_M_parser.pathlen > 0) {
    _M_hier_part.path.set(d + (_M_parser.path - _M_parser.begin),
                          _M_parser.pathlen);
  }

  if (_M_parser.querylen > 0) {
    _M_query.set(d + (_M_parser.query - _M_parser.begin), _M_parser.querylen);
  }

  if (_M_parser.fragmentlen > 0) {
    _M_fragment.set(d + (_M_parser.fragment - _M_parser.begin),
                    _M_parser.fragmentlen);
  }

  return true;
}

bool net::uri::uri::parser::parse_scheme()
{
  // scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
  if (!util::is_alpha(*p)) {
    return false;
  }

  while (++p < end) {
    uint8_t c = *p;
    if (!is_valid_scheme_char(c)) {
      if (c == ':') {
        schemelen = p - begin;
        return true;
      } else {
        return false;
      }
    }
  }

  return false;
}

bool net::uri::uri::parser::parse_authority()
{
  const uint8_t* colon = NULL;
  unsigned ncolons = 0;
  const uint8_t* at = NULL;
  bool authority = true;

  // Save possible start of the host.
  host = p;

  do {
    if (!is_valid_reg_name_char(p, end)) {
      switch (*p) {
        case ':':
          colon = p;
          ncolons++;
          break;
        case '@':
          if (at) {
            return false;
          }

          at = p;

          colon = NULL;
          ncolons = 0;

          break;
        case '[':
          if (at) {
            // Save start of the userinfo.
            userinfo = host;

            userinfolen = at - userinfo;

            if (at + 1 != p) {
              return false;
            }
          } else if (p != host) {
            return false;
          }

          return parse_ip_literal();
        case '/':
        case '?':
        case '#':
          authority = false;
          break;
        default:
          return false;
      }
    }
  } while ((authority) && (++p < end));

  if (at) {
    // Save start of the userinfo.
    userinfo = host;

    userinfolen = at - userinfo;

    // Save start of the host.
    host = at + 1;
  }

  const uint8_t* hostend;
  switch (ncolons) {
    case 0:
      hostend = p;
      break;
    case 1:
      // If the port is not empty...
      if (colon + 1 < p) {
        // Parse port.
        uint32_t n;
        if (util::number::parse(colon + 1, p - (colon + 1), n, 1, USHRT_MAX) !=
            util::number::parse_result::kSucceeded) {
          return false;
        }

        port = (standard_port(reinterpret_cast<const char*>(begin),
                              schemelen) == n) ?
                                                 0 :
                                                 static_cast<in_port_t>(n);
      }

      hostend = colon;

      break;
    default:
      return false;
  }

  hostlen = hostend - host;

  return ((hostlen > 0) || ((userinfolen == 0) && (port == 0)));
}

bool net::uri::uri::parser::parse_ip_literal()
{
  host = ++p;

  while ((p < end) && (is_valid_ip_literal_char(*p))) {
    p++;
  }

  if ((p == end) || (*p != ']')) {
    return false;
  }

  if ((hostlen = p - host) == 0) {
    return false;
  }

  if (++p == end) {
    return true;
  }

  // Port?
  if (*p == ':') {
    // Save position of the colon.
    const uint8_t* colon = p;

    uint32_t n = 0;
    while ((++p < end) && (util::is_digit(*p))) {
      if ((n = (n * 10) + (*p - '0')) > USHRT_MAX) {
        return false;
      }
    }

    // If the port is not empty...
    if (colon + 1 < p) {
      if (n == 0) {
        return false;
      }

      port = (standard_port(reinterpret_cast<const char*>(begin),
                            schemelen) == n) ?
                                               0 :
                                               static_cast<in_port_t>(n);
    }
  }

  ip_literal = true;

  return true;
}

bool net::uri::uri::parser::parse_path()
{
  while ((p < end) && (is_valid_path_char(p, end))) {
    // Double slash?
    if ((p > path) && (*p == '/') && (p[-1] == '/')) {
      return false;
    }

    p++;
  }

  pathlen = p - path;

  return true;
}

void net::uri::uri::parser::parse_query()
{
  while ((p < end) && (is_valid_query_or_fragment_char(p, end))) {
    p++;
  }

  querylen = p - query;
}

void net::uri::uri::parser::parse_fragment()
{
  while ((p < end) && (is_valid_query_or_fragment_char(p, end))) {
    p++;
  }

  fragmentlen = p - fragment;
}
