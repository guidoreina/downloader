#ifndef STRING_MEMCASEMEM_H
#define STRING_MEMCASEMEM_H

namespace string {
  void* memcasemem(const void* haystack,
                   size_t haystacklen,
                   const void* needle,
                   size_t needlelen);
}

#endif // STRING_MEMCASEMEM_H
