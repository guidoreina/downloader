#ifndef STRING_SLICE_H
#define STRING_SLICE_H

#include <string.h>
#include "string/buffer.h"
#include "macros/macros.h"

namespace string {
  class slice {
    public:
      // Constructor.
      slice();
      slice(const slice& other);
      slice(slice&& other);
      slice(const char* str, size_t len);
      slice(const buffer& buf);

      // Assignment operator.
      slice& operator=(const slice& other);

      // Move assignment operator.
      slice& operator=(slice&& other);

      // Swap content.
      void swap(slice& other);

      // Clear.
      void clear();

      // Set.
      void set(const char* str, size_t len);

      // Get data.
      const char* data() const;

      // Get length.
      size_t length() const;

      // Compare.
      int compare(const slice& other) const;

    private:
      const char* _M_data;
      size_t _M_len;
  };

  inline slice::slice()
    : _M_data(""),
      _M_len(0)
  {
  }

  inline slice::slice(const slice& other)
    : _M_data(other._M_data),
      _M_len(other._M_len)
  {
  }

  inline slice::slice(slice&& other)
    : _M_data(other._M_data),
      _M_len(other._M_len)
  {
    other.clear();
  }

  inline slice::slice(const char* str, size_t len)
    : _M_data(str),
      _M_len(len)
  {
  }

  inline slice::slice(const buffer& buf)
    : _M_data(buf.data()),
      _M_len(buf.length())
  {
  }

  inline slice& slice::operator=(const slice& other)
  {
    _M_data = other._M_data;
    _M_len = other._M_len;

    return *this;
  }

  inline slice& slice::operator=(slice&& other)
  {
    _M_data = other._M_data;
    _M_len = other._M_len;

    other.clear();

    return *this;
  }

  inline void slice::swap(slice& other)
  {
    const char* data = _M_data;
    _M_data = other._M_data;
    other._M_data = data;

    size_t len = _M_len;
    _M_len = other._M_len;
    other._M_len = len;
  }

  inline void slice::clear()
  {
    _M_data = "";
    _M_len = 0;
  }

  inline void slice::set(const char* str, size_t len)
  {
    _M_data = str;
    _M_len = len;
  }

  inline const char* slice::data() const
  {
    return _M_data;
  }

  inline size_t slice::length() const
  {
    return _M_len;
  }

  inline int slice::compare(const slice& other) const
  {
    size_t minlen = MIN(_M_len, other._M_len);

    int ret;
    if ((ret = memcmp(_M_data, other._M_data, minlen)) != 0) {
      return ret;
    }

    return _M_len - other._M_len;
  }
}

#endif // STRING_SLICE_H
