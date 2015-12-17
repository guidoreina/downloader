#include <unistd.h>
#include "net/poll_selector.h"

bool net::selector::create()
{
  if (!_M_fdmap.create()) {
    return false;
  }

  if ((_M_events = reinterpret_cast<struct pollfd*>(
                     malloc(_M_fdmap.size() * sizeof(struct pollfd))
                   )) == NULL) {
    return false;
  }

  return true;
}

bool net::selector::add(unsigned fd,
                        fdtype type,
                        io::event_handler* handler,
                        io::event events)
{
  if (!_M_fdmap.add(fd, type, handler)) {
    // The file descriptor has been already inserted.
    return true;
  }

  size_t index = _M_fdmap.count() - 1;

  _M_events[index].fd = fd;
  _M_events[index].revents = 0;

  switch (events) {
    case io::event::kRead:
#if HAVE_POLLRDHUP
      _M_events[index].events = POLLIN | POLLRDHUP;
#else
      _M_events[index].events = POLLIN;
#endif

      break;
    case io::event::kWrite:
      _M_events[index].events = POLLOUT;
      break;
    default:
#if HAVE_POLLRDHUP
      _M_events[index].events = POLLIN | POLLRDHUP | POLLOUT;
#else
      _M_events[index].events = POLLIN | POLLOUT;
#endif
  }

  return true;
}

bool net::selector::remove(unsigned fd)
{
  int index;
  if ((index = _M_fdmap.index(fd)) < 0) {
    // The file descriptor has not been inserted.
    return true;
  }

  _M_fdmap.remove(fd);

  if (static_cast<size_t>(index) < _M_fdmap.count()) {
    _M_events[index] = _M_events[_M_fdmap.count()];
  }

  close(fd);

  return true;
}

bool net::selector::process_fd_events(unsigned fd,
                                      fdtype type,
                                      io::event_handler* handler,
                                      io::event events)
{
  switch (handler->on_io(events)) {
    case io::event_handler::result::kError:
      if (_M_io_observer) {
        _M_io_observer->on_error(handler);
      }

      return false;
    case io::event_handler::result::kChangeToReadMode:
#if HAVE_POLLRDHUP
      _M_events[_M_fdmap.index(fd)].events = POLLIN | POLLRDHUP;
#else
      _M_events[_M_fdmap.index(fd)].events = POLLIN;
#endif

      break;
    case io::event_handler::result::kChangeToWriteMode:
      _M_events[_M_fdmap.index(fd)].events = POLLOUT;
      break;
    default:
      ;
  }

  if (_M_io_observer) {
    _M_io_observer->on_success(handler);
  }

  return true;
}

void net::selector::process_events()
{
  // For each file descriptor...
  size_t i = 0;
  while ((i < _M_fdmap.count()) && (_M_nevents > 0)) {
    unsigned fd = _M_events[i].fd;

    short ev = _M_events[i].revents;

    // If an error happened but POLLIN / POLLOUT were not set...
    if ((ev & (POLLERR | POLLHUP | POLLNVAL)) &&
        ((ev & (POLLIN | POLLOUT)) == 0)) {
      ev |= (POLLIN | POLLOUT);
    }

    io::event events = io::event::kNoEvent;

    if (ev & POLLIN) {
      events = static_cast<io::event>(
                 static_cast<unsigned>(events) |
                 static_cast<unsigned>(io::event::kRead)
               );
    }

    if (ev & POLLOUT) {
      events = static_cast<io::event>(
                 static_cast<unsigned>(events) |
                 static_cast<unsigned>(io::event::kWrite)
               );
    }

    fdtype type;
    io::event_handler* handler;
    if (events != io::event::kNoEvent) {
      if ((_M_fdmap.get(fd, type, handler)) &&
          (!process_fd_events(fd, type, handler, events))) {
        remove(fd);
      } else {
        i++;
      }

      _M_nevents--;
    } else {
      i++;
    }
  }
}
