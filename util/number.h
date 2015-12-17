#ifndef UTIL_NUMBER_H
#define UTIL_NUMBER_H

#include <sys/types.h>
#include <stdint.h>
#include <limits.h>

namespace util {
  class number {
    public:
      static size_t length(off_t number);

      // Parse.
      enum class parse_result {
        kError,
        kUnderflow,
        kOverflow,
        kSucceeded
      };

      static parse_result parse(const void* buf,
                                size_t len,
                                int32_t& n,
                                int32_t min = INT_MIN,
                                int32_t max = INT_MAX);

      static parse_result parse(const void* buf,
                                size_t len,
                                uint32_t& n,
                                uint32_t min = 0,
                                uint32_t max = UINT_MAX);

      static parse_result parse(const void* buf,
                                size_t len,
                                int64_t& n,
                                int64_t min = LLONG_MIN,
                                int64_t max = LLONG_MAX);

      static parse_result parse(const void* buf,
                                size_t len,
                                uint64_t& n,
                                uint64_t min = 0,
                                uint64_t max = ULLONG_MAX);
  };

  inline number::parse_result number::parse(const void* buf,
                                            size_t len,
                                            int32_t& n,
                                            int32_t min,
                                            int32_t max)
  {
    int64_t int64;
    parse_result res;
    if ((res = parse(buf,
                     len,
                     int64,
                     min,
                     max)) != parse_result::kSucceeded) {
      return res;
    }

    n = static_cast<int32_t>(int64);

    return parse_result::kSucceeded;
  }

  inline number::parse_result number::parse(const void* buf,
                                            size_t len,
                                            uint32_t& n,
                                            uint32_t min,
                                            uint32_t max)
  {
    uint64_t uint64;
    parse_result res;
    if ((res = parse(buf,
                     len,
                     uint64,
                     min,
                     max)) != parse_result::kSucceeded) {
      return res;
    }

    n = static_cast<uint32_t>(uint64);

    return parse_result::kSucceeded;
  }
}

#endif // UTIL_NUMBER_H
