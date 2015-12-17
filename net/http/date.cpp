#include <stdlib.h>
#include "net/http/date.h"

time_t net::http::date::parse(const void* buf, size_t len, struct tm& timestamp)
{
  // RFC 2616
  // http://www.faqs.org/rfcs/rfc2616.html
  // 3.3.1 Full Date
  // HTTP applications have historically allowed three different formats
  // for the representation of date/time stamps:

  // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
  // Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
  // Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format

  enum {
    kRfc1123,
    kRfc850,
    kAnsiC
  } date_format;

  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(buf);
  const uint8_t* end = ptr + len;

  // Skip day of the week.
  while ((ptr < end) && (util::is_alpha(*ptr))) {
    ptr++;
  }

  if (ptr == end) {
    return static_cast<time_t>(-1);
  }

  size_t n = ptr - reinterpret_cast<const uint8_t*>(buf);

  if (*ptr == ' ') {
    if (n != 3) {
      return static_cast<time_t>(-1);
    }

    ptr++;

    if (static_cast<size_t>(end - ptr) < sizeof("Nov 6 08:49:37 1994") - 1) {
      return static_cast<time_t>(-1);
    }

    return _M_parse_ansic(ptr, end, timestamp);
  }

  if ((*ptr != ',') || (n < 3) || (n > 9)) {
    return static_cast<time_t>(-1);
  }

  if (n == 3) {
    date_format = kRfc1123;
  } else {
    date_format = kRfc850;
  }

  ptr++;

  if (static_cast<size_t>(end - ptr) < sizeof("06-Nov-94 08:49:37 GMT") - 1) {
    return static_cast<time_t>(-1);
  }

  // Skip white space (if any).
  if (*ptr == ' ') {
    ptr++;
  }

  // Parse day of the month.
  if ((!util::is_digit(*ptr)) || (!util::is_digit(*(ptr + 1)))) {
    return static_cast<time_t>(-1);
  }

  unsigned mday;
  if (((mday = ((*ptr - '0') * 10) + (*(ptr + 1) - '0')) < 1) || (mday > 31)) {
    return static_cast<time_t>(-1);
  }

  ptr += 2;

  if (date_format == kRfc1123) {
    if (*ptr != ' ') {
      return static_cast<time_t>(-1);
    }
  } else {
    if (*ptr != '-') {
      return static_cast<time_t>(-1);
    }
  }

  // Parse month.
  unsigned mon;
  if (!_M_parse_month(++ptr, mon)) {
    return static_cast<time_t>(-1);
  }

  ptr += 3;

  // Parse year.
  unsigned year;
  if (date_format == kRfc1123) {
    if (*ptr != ' ') {
      return static_cast<time_t>(-1);
    }

    if (!_M_parse_year(++ptr, year)) {
      return static_cast<time_t>(-1);
    }

    ptr += 4;
  } else {
    if (*ptr != '-') {
      return static_cast<time_t>(-1);
    }

    ptr++;

    if ((!util::is_digit(*ptr)) || (!util::is_digit(*(ptr + 1)))) {
      return static_cast<time_t>(-1);
    }

    if ((year = ((*ptr - '0') * 10) + (*(ptr + 1) - '0')) < 70) {
      year += 2000;
    } else {
      year += 1900;
    }

    ptr += 2;
  }

  if (*ptr != ' ') {
    return static_cast<time_t>(-1);
  }

  if (++ptr + sizeof("08:49:37 GMT") - 1 != end) {
    return static_cast<time_t>(-1);
  }

  // Parse time.
  unsigned hour;
  unsigned min;
  unsigned sec;
  if (!_M_parse_time(ptr, hour, min, sec)) {
    return static_cast<time_t>(-1);
  }

  ptr += 9;

  if (((*ptr != 'G') && (*ptr != 'g')) ||
      ((*(ptr + 1) != 'M') && (*(ptr + 1) != 'm')) ||
      ((*(ptr + 2) != 'T') && (*(ptr + 2) != 't'))) {
    return static_cast<time_t>(-1);
  }

  timestamp.tm_year = year - 1900;
  timestamp.tm_mon = mon;
  timestamp.tm_mday = mday;
  timestamp.tm_hour = hour;
  timestamp.tm_min = min;
  timestamp.tm_sec = sec;
  timestamp.tm_isdst = 0;

#ifdef HAVE_TIMEGM
  return timegm(&timestamp);
#elif HAVE_TIMEZONE
  return mktime(&timestamp) - timezone;
#else
  return mktime(&timestamp);
#endif
}

time_t net::http::date::_M_parse_ansic(const uint8_t* begin,
                                       const uint8_t* end,
                                       struct tm& timestamp)
{
  const uint8_t* ptr = begin;

  // Parse month.
  unsigned mon;
  if (!_M_parse_month(ptr, mon)) {
    return static_cast<time_t>(-1);
  }

  if (*(ptr += 3) != ' ') {
    return static_cast<time_t>(-1);
  }

  ptr++;

  // Parse day of the month.
  unsigned mday;
  if (util::is_digit(*ptr)) {
    mday = *ptr - '0';
  } else if (*ptr == ' ') {
    mday = 0;
  } else {
    return static_cast<time_t>(-1);
  }

  ptr++;

  if (util::is_digit(*ptr)) {
    mday = (mday * 10) + (*ptr - '0');
    ptr++;
  }

  if ((mday < 1) || (mday > 31)) {
    return static_cast<time_t>(-1);
  }

  if (*ptr != ' ') {
    return static_cast<time_t>(-1);
  }

  if (++ptr + sizeof("08:49:37 1994") - 1 != end) {
    return static_cast<time_t>(-1);
  }

  // Parse time.
  unsigned hour;
  unsigned min;
  unsigned sec;
  if (!_M_parse_time(ptr, hour, min, sec)) {
    return static_cast<time_t>(-1);
  }

  // Parse year.
  unsigned year;
  if (!_M_parse_year(ptr += 9, year)) {
    return static_cast<time_t>(-1);
  }

  timestamp.tm_year = year - 1900;
  timestamp.tm_mon = mon;
  timestamp.tm_mday = mday;
  timestamp.tm_hour = hour;
  timestamp.tm_min = min;
  timestamp.tm_sec = sec;
  timestamp.tm_isdst = 0;

#ifdef HAVE_TIMEGM
  return timegm(&timestamp);
#elif HAVE_TIMEZONE
  return mktime(&timestamp) - timezone;
#else
  return mktime(&timestamp);
#endif
}

bool net::http::date::_M_parse_month(const uint8_t* ptr, unsigned& mon)
{
  switch (*ptr) {
    case 'J':
    case 'j':
      if ((*(ptr + 1) == 'a') || (*(ptr + 1) == 'A')) {
        if ((*(ptr + 2) == 'n') || (*(ptr + 2) == 'N')) {
          mon = 0;
        } else {
          return false;
        }
      } else if ((*(ptr + 1) == 'u') || (*(ptr + 1) == 'U')) {
        if ((*(ptr + 2) == 'n') || (*(ptr + 2) == 'N')) {
          mon = 5;
        } else if ((*(ptr + 2) == 'l') || (*(ptr + 2) == 'L')) {
          mon = 6;
        } else {
          return false;
        }
      } else {
        return false;
      }

      break;
    case 'F':
    case 'f':
      if (((*(ptr + 1) == 'e') || (*(ptr + 1) == 'E')) &&
          ((*(ptr + 2) == 'b') || (*(ptr + 2) == 'B'))) {
        mon = 1;
      } else {
        return false;
      }

      break;
    case 'M':
    case 'm':
      if ((*(ptr + 1) == 'a') || (*(ptr + 1) == 'A')) {
        if ((*(ptr + 2) == 'r') || (*(ptr + 2) == 'R')) {
          mon = 2;
        } else if ((*(ptr + 2) == 'y') || (*(ptr + 2) == 'Y')) {
          mon = 4;
        } else {
          return false;
        }
      } else {
        return false;
      }

      break;
    case 'A':
    case 'a':
      if ((*(ptr + 1) == 'p') || (*(ptr + 1) == 'P')) {
        if ((*(ptr + 2) == 'r') || (*(ptr + 2) == 'R')) {
          mon = 3;
        } else {
          return false;
        }
      } else if ((*(ptr + 1) == 'u') || (*(ptr + 1) == 'U')) {
        if ((*(ptr + 2) == 'g') || (*(ptr + 2) == 'G')) {
          mon = 7;
        } else {
          return false;
        }
      } else {
        return false;
      }

      break;
    case 'S':
    case 's':
      if (((*(ptr + 1) == 'e') || (*(ptr + 1) == 'E')) &&
          ((*(ptr + 2) == 'p') || (*(ptr + 2) == 'P'))) {
        mon = 8;
      } else {
        return false;
      }

      break;
    case 'O':
    case 'o':
      if (((*(ptr + 1) == 'c') || (*(ptr + 1) == 'C')) &&
          ((*(ptr + 2) == 't') || (*(ptr + 2) == 'T'))) {
        mon = 9;
      } else {
        return false;
      }

      break;
    case 'N':
    case 'n':
      if (((*(ptr + 1) == 'o') || (*(ptr + 1) == 'O')) &&
          ((*(ptr + 2) == 'v') || (*(ptr + 2) == 'V'))) {
        mon = 10;
      } else {
        return false;
      }

      break;
    case 'D':
    case 'd':
      if (((*(ptr + 1) == 'e') || (*(ptr + 1) == 'E')) &&
          ((*(ptr + 2) == 'c') || (*(ptr + 2) == 'C'))) {
        mon = 11;
      } else {
        return false;
      }

      break;
    default:
      return false;
  }

  return true;
}

bool net::http::date::_M_parse_time(const uint8_t* ptr,
                                    unsigned& hour,
                                    unsigned& min,
                                    unsigned& sec)
{
  // Parse hour.
  if ((!util::is_digit(*ptr)) || (!util::is_digit(*(ptr + 1)))) {
    return false;
  }

  if ((hour = ((*ptr - '0') * 10) + (*(ptr + 1) - '0')) > 23) {
    return false;
  }

  if (*(ptr += 2) != ':') {
    return false;
  }

  ptr++;

  // Parse minutes.
  if ((!util::is_digit(*ptr)) || (!util::is_digit(*(ptr + 1)))) {
    return false;
  }

  if ((min = ((*ptr - '0') * 10) + (*(ptr + 1) - '0')) > 59) {
    return false;
  }

  if (*(ptr += 2) != ':') {
    return false;
  }

  ptr++;

  // Parse seconds.
  if ((!util::is_digit(*ptr)) || (!util::is_digit(*(ptr + 1)))) {
    return false;
  }

  if ((sec = ((*ptr - '0') * 10) + (*(ptr + 1) - '0')) > 59) {
    return false;
  }

  return (*(ptr += 2) == ' ');
}
