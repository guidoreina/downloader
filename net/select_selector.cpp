#include <stdlib.h>
#include <unistd.h>
#include "net/select_selector.h"

bool net::selector::add(unsigned fd,
                        fdtype type,
                        io::event_handler* handler,
                        io::event events)
{
  if (!_M_fdmap.add(fd, type, handler)) {
    // The file descriptor has been already inserted.
    return true;
  }

  if (static_cast<unsigned>(events) & static_cast<unsigned>(io::event::kRead)) {
    FD_SET(fd, &_M_rfds);
  }

  if (static_cast<unsigned>(events) & static_cast<unsigned>(io::event::kWrite)) {
    FD_SET(fd, &_M_wfds);
  }

  if ((_M_max_fd != kUndefined) && (static_cast<int>(fd) > _M_max_fd)) {
    _M_max_fd = fd;
  }

  return true;
}

bool net::selector::remove(unsigned fd)
{
  if (!_M_fdmap.remove(fd)) {
    // The file descriptor has not been inserted.
    return true;
  }

  FD_CLR(fd, &_M_rfds);
  FD_CLR(fd, &_M_wfds);

  // If the file descriptor was the highest-numbered file descriptor...
  if (static_cast<int>(fd) == _M_max_fd) {
    _M_max_fd = kUndefined;
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
      FD_CLR(fd, &_M_wfds);
      FD_SET(fd, &_M_rfds);
      break;
    case io::event_handler::result::kChangeToWriteMode:
      FD_CLR(fd, &_M_rfds);
      FD_SET(fd, &_M_wfds);
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
    unsigned fd = _M_fdmap.fd(i);
    io::event events = io::event::kNoEvent;

    if (FD_ISSET(fd, &_M_tmp_rfds)) {
      events = static_cast<io::event>(
                 static_cast<unsigned>(events) |
                 static_cast<unsigned>(io::event::kRead)
               );

      _M_nevents--;
    }

    if (FD_ISSET(fd, &_M_tmp_wfds)) {
      events = static_cast<io::event>(
                 static_cast<unsigned>(events) |
                 static_cast<unsigned>(io::event::kWrite)
               );

      _M_nevents--;
    }

    fdtype type;
    io::event_handler* handler;
    if ((events != io::event::kNoEvent) &&
        (_M_fdmap.get(fd, type, handler)) &&
        (!process_fd_events(fd, type, handler, events))) {
      remove(fd);
    } else {
      i++;
    }
  }
}
