#include <stdlib.h>
#include <string.h>
#include "net/http/methods.h"
#include "macros/macros.h"

const struct net::http::methods::name net::http::methods::_M_names[] = {
  {"CONNECT",   7},
  {"COPY",      4},
  {"DELETE",    6},
  {"GET",       3},
  {"HEAD",      4},
  {"LOCK",      4},
  {"MKCOL",     5},
  {"MOVE",      4},
  {"OPTIONS",   7},
  {"POST",      4},
  {"PROPFIND",  8},
  {"PROPPATCH", 9},
  {"PUT",       3},
  {"TRACE",     5},
  {"UNLOCK",    6}
};

net::http::method net::http::methods::search(const char* s, size_t len)
{
  int i = 0;
  int j = ARRAY_SIZE(_M_names) - 1;

  while (i <= j) {
    int pivot = (i + j) / 2;

    int ret;
    if ((ret = strncmp(s, _M_names[pivot].name, len)) < 0) {
      j = pivot - 1;
    } else if (ret == 0) {
      if (len < _M_names[pivot].len) {
        j = pivot - 1;
      } else if (len == _M_names[pivot].len) {
        return static_cast<method>(pivot);
      } else {
        i = pivot + 1;
      }
    } else {
      i = pivot + 1;
    }
  }

  return method::kUnknown;
}
