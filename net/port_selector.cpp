#include <poll.h>
#include "net/port_selector.h"

net::selector::~selector()
{
  if (_M_fd != -1) {
    close(_M_fd);
  }

  if (_M_port_events) {
    free(_M_port_events);
  }

  if (_M_events) {
    free(_M_events);
  }
}

bool net::selector::create()
{
  if (!_M_fdmap.create()) {
    return false;
  }

  if ((_M_fd = port_create()) < 0) {
    return false;
  }

  if ((_M_port_events = reinterpret_cast<port_event_t*>(
                          malloc(_M_fdmap.size() * sizeof(port_event_t))
                        )) == NULL) {
    return false;
  }

  if ((_M_events = reinterpret_cast<int*>(
                     malloc(_M_fdmap.size() * sizeof(int))
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

  int ev;
  switch (events) {
    case io::event::kRead:
      ev = POLLIN;
      break;
    case io::event::kWrite:
      ev = POLLOUT;
      break;
    default:
      ev = POLLIN | POLLOUT;
  }

  if (port_associate(_M_fd,
                     PORT_SOURCE_FD,
                     static_cast<uintptr_t>(fd),
                     ev,
                     NULL) < 0) {
    _M_fdmap.remove(fd);
    return false;
  }

  _M_events[fd] = ev;

  return true;
}

bool net::selector::remove(unsigned fd)
{
  if (!_M_fdmap.remove(fd)) {
    // The file descriptor has not been inserted.
    return true;
  }

  if (port_dissociate(_M_fd, PORT_SOURCE_FD, static_cast<uintptr_t>(fd)) < 0) {
    close(fd);
    return false;
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
      _M_events[fd] = POLLIN;
      break;
    case io::event_handler::result::kChangeToWriteMode:
      _M_events[fd] = POLLOUT;
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
  // For each event...
  for (unsigned i = 0; i < _M_nevents; i++) {
    unsigned fd = static_cast<unsigned>(_M_port_events[i].portev_object);

    fdtype type;
    io::event_handler* handler;
    if (_M_fdmap.get(fd, type, handler)) {
      io::event events = io::event::kNoEvent;

      if (_M_port_events[i].portev_events & POLLIN) {
        events = static_cast<io::event>(
                   static_cast<unsigned>(events) |
                   static_cast<unsigned>(io::event::kRead)
                 );
      }

      if (_M_port_events[i].portev_events & POLLOUT) {
        events = static_cast<io::event>(
                   static_cast<unsigned>(events) |
                   static_cast<unsigned>(io::event::kWrite)
                 );
      }

      if (!process_fd_events(fd, type, handler, events)) {
        _M_fdmap.remove(fd);
        close(fd);
      } else {
        // Reassociate file descriptor.
        if (port_associate(_M_fd,
                           PORT_SOURCE_FD,
                           static_cast<uintptr_t>(fd),
                           _M_events[fd],
                           NULL) < 0) {
          _M_fdmap.remove(fd);
          close(fd);
        }
      }
    }
  }
}
