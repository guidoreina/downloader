#include <stdlib.h>
#include <string.h>
#include "net/ports.h"

in_port_t net::standard_port(const char* scheme, size_t len)
{
  switch (len) {
    case 3:
      if (strncasecmp(scheme, "ftp", 3) == 0) {
        return 21;
      }

      break;
    case 4:
      if (strncasecmp(scheme, "http", 4) == 0) {
        return 80;
      }

      break;
    case 5:
      if (strncasecmp(scheme, "https", 5) == 0) {
        return 443;
      }

      break;
  }

  return 0;
}
