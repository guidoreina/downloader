#include "util/ranges.h"

void util::ranges::free()
{
  if (_M_ranges) {
    ::free(_M_ranges);
    _M_ranges = NULL;
  }

  _M_size = 0;
  _M_used = 0;
}

bool util::ranges::add(off_t from, off_t to)
{
  if (_M_used == _M_size) {
    size_t size = (_M_size == 0) ? kInitialSize : (_M_size * 2);
    struct range* ranges;
    if ((ranges = reinterpret_cast<struct range*>(
                    realloc(_M_ranges, size * sizeof(struct range))
                  )) == NULL) {
      return false;
    }

    _M_ranges = ranges;
    _M_size = size;
  }

  _M_ranges[_M_used].from = from;
  _M_ranges[_M_used].to = to;

  _M_used++;

  return true;
}
