#include <stdlib.h>
#include <stdint.h>
#include "string/memcasemem.h"
#include "util/ctype.h"

void* string::memcasemem(const void* haystack,
                         size_t haystacklen,
                         const void* needle,
                         size_t needlelen)
{
  if (needlelen == 0) {
    return const_cast<void*>(haystack);
  }

  if (haystacklen < needlelen) {
    return NULL;
  }

  const uint8_t* end = reinterpret_cast<const uint8_t*>(haystack) +
                       haystacklen -
                       needlelen;

  const uint8_t* n = reinterpret_cast<const uint8_t*>(needle);

  for (const uint8_t* ptr = reinterpret_cast<const uint8_t*>(haystack);
       ptr <= end;
       ptr++) {
    size_t i;
    for (i = 0; i < needlelen; i++) {
      if (util::to_lower(ptr[i]) != util::to_lower(n[i])) {
        break;
      }
    }

    if (i == needlelen) {
      return reinterpret_cast<void*>(const_cast<uint8_t*>(ptr));
    }
  }

  return NULL;
}
