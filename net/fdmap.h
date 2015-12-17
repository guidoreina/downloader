#ifndef NET_FDMAP_H
#define NET_FDMAP_H

#include <stdlib.h>
#include <stdint.h>
#include "io/event_handler.h"

namespace net {
  enum class fdtype : uint8_t {
    kFdNone,
    kFdSocket,
    kFdListener
  };

  class fdmap {
    public:
      // Constructor.
      fdmap();

      // Destructor.
      ~fdmap();

      // Create.
      bool create();

      // Get size.
      size_t size() const;

      // Get count.
      size_t count() const;

      // Add descriptor.
      bool add(unsigned fd, fdtype type, io::event_handler* handler);

      // Remove descriptor.
      bool remove(unsigned fd);

      // Get index.
      int index(unsigned fd) const;

      // Get type.
      fdtype type(unsigned fd) const;

      // Get I/O event handler.
      io::event_handler* handler(unsigned fd) const;

      // Get type and event handler.
      bool get(unsigned fd, fdtype& type, io::event_handler*& handler) const;

      // Get descriptor.
      int fd(unsigned index) const;

    private:
      struct fdentry {
        int index;
        fdtype type;
        io::event_handler* handler;
      };

      struct fdentry* _M_entries;
      size_t _M_size;
      size_t _M_used;

      unsigned* _M_index;

      // Disable copy constructor and assignment operator.
      fdmap(const fdmap&) = delete;
      fdmap& operator=(const fdmap&) = delete;
  };

  inline fdmap::fdmap()
    : _M_entries(NULL),
      _M_size(0),
      _M_used(0),
      _M_index(NULL)
  {
  }

  inline size_t fdmap::size() const
  {
    return _M_size;
  }

  inline size_t fdmap::count() const
  {
    return _M_used;
  }

  inline int fdmap::index(unsigned fd) const
  {
    return _M_entries[fd].index;
  }

  inline fdtype fdmap::type(unsigned fd) const
  {
    return (_M_entries[fd].index != -1) ? _M_entries[fd].type : fdtype::kFdNone;
  }

  inline io::event_handler* fdmap::handler(unsigned fd) const
  {
    return (_M_entries[fd].index != -1) ? _M_entries[fd].handler : NULL;
  }

  inline bool fdmap::get(unsigned fd,
                         fdtype& type,
                         io::event_handler*& handler) const
  {
    if (_M_entries[fd].index != -1) {
      type = _M_entries[fd].type;
      handler = _M_entries[fd].handler;

      return true;
    }

    return false;
  }

  inline int fdmap::fd(unsigned index) const
  {
    return (index < _M_used) ? _M_index[index] : -1;
  }
}

#endif // NET_FDMAP_H
