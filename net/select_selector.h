#ifndef NET_SELECT_SELECTOR_H
#define NET_SELECT_SELECTOR_H

#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include "net/fdmap.h"
#include "io/observer.h"
#include "io/event.h"

namespace net {
  class selector {
    public:
      // Constructor.
      selector();

      // Destructor.
      ~selector();

      // Set I/O observer.
      void set_io_observer(io::observer* obs);

      // Get size.
      size_t size() const;

      // Get count.
      size_t count() const;

      // Create.
      bool create();

      // Add descriptor.
      bool add(unsigned fd,
               fdtype type,
               io::event_handler* handler,
               io::event events);

      // Remove descriptor.
      bool remove(unsigned fd);

      // Wait for events.
      bool wait_for_events();
      bool wait_for_events(unsigned timeout); // Timeout in ms.

      // Process file descriptor's events.
      bool process_fd_events(unsigned fd,
                             fdtype type,
                             io::event_handler* handler,
                             io::event events);

      // Process all events.
      void process_events();

    private:
      static const int kUndefined = -2;
      static const int kEmptySet = -1;

      fdmap _M_fdmap;

      fd_set _M_rfds;
      fd_set _M_wfds;

      fd_set _M_tmp_rfds;
      fd_set _M_tmp_wfds;

      int _M_max_fd;

      unsigned _M_nevents;

      io::observer* _M_io_observer;

      // Compute highest file descriptor.
      int _M_highest_fd();

      // Disable copy constructor and assignment operator.
      selector(const selector&) = delete;
      selector& operator=(const selector&) = delete;
  };

  inline selector::selector()
    : _M_max_fd(kEmptySet),
      _M_io_observer(NULL)
  {
    FD_ZERO(&_M_rfds);
    FD_ZERO(&_M_wfds);
  }

  inline selector::~selector()
  {
  }

  inline void selector::set_io_observer(io::observer* obs)
  {
    _M_io_observer = obs;
  }

  inline size_t selector::size() const
  {
    return _M_fdmap.size();
  }

  inline size_t selector::count() const
  {
    return _M_fdmap.count();
  }

  inline bool selector::create()
  {
    return _M_fdmap.create();
  }

  inline bool selector::wait_for_events()
  {
    _M_tmp_rfds = _M_rfds;
    _M_tmp_wfds = _M_wfds;

    int ret;
    if ((ret = select(_M_highest_fd() + 1,
                      &_M_tmp_rfds,
                      &_M_tmp_wfds,
                      NULL,
                      NULL)) <= 0) {
      return false;
    }

    _M_nevents = ret;

    return true;
  }

  inline bool selector::wait_for_events(unsigned timeout)
  {
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    _M_tmp_rfds = _M_rfds;
    _M_tmp_wfds = _M_wfds;

    int ret;
    if ((ret = select(_M_highest_fd() + 1,
                      &_M_tmp_rfds,
                      &_M_tmp_wfds,
                      NULL,
                      &tv)) <= 0) {
      return false;
    }

    _M_nevents = ret;

    return true;
  }

  inline int selector::_M_highest_fd()
  {
    if (_M_max_fd == kUndefined) {
      size_t count = _M_fdmap.count();
      for (size_t i = 0; i < count; i++) {
        int fd;
        if ((fd = _M_fdmap.fd(i)) > _M_max_fd) {
          _M_max_fd = fd;
        }
      }
    }

    return _M_max_fd;
  }
}

#endif // NET_SELECT_SELECTOR_H
