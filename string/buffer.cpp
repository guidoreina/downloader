#include <stdio.h>
#include "string/buffer.h"

bool string::buffer::allocate(size_t size)
{
  if ((size += _M_used) <= _M_size) {
    return true;
  }

  size_t s;
  if (_M_size == 0) {
    s = kInitialSize;
  } else {
    size_t tmp;
    if ((tmp = _M_size * 2) < _M_size) {
      // Overflow.
      return false;
    }

    s = tmp;
  }

  while (s < size) {
    size_t tmp;
    if ((tmp = s * 2) < s) {
      // Overflow.
      return false;
    }

    s = tmp;
  }

  char* data;
  if ((data = reinterpret_cast<char*>(realloc(_M_data, s))) == NULL) {
    return false;
  }

  _M_data = data;
  _M_size = s;

  return true;
}

bool string::buffer::vformat(const char* format, va_list ap)
{
  if (!allocate(kInitialSize)) {
    return false;
  }

  int size = _M_size - _M_used;

  do {
    va_list aq;
    va_copy(aq, ap);
    int n = vsnprintf(_M_data + _M_used, size, format, aq);
    va_end(aq);

    if (n > -1) {
      if (n < size) {
        _M_used += n;
        return true;
      }

      size = n + 1;
    } else {
      size *= 2;
    }

    if (!allocate(size)) {
      return false;
    }
  } while (true);
}
