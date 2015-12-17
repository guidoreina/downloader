#include <unistd.h>
#include "net/epoll_selector.h"

net::selector::~selector()
{
  if (_M_fd != -1) {
    close(_M_fd);
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

  if ((_M_fd = epoll_create(_M_fdmap.size())) < 0) {
    return false;
  }

  if ((_M_events = reinterpret_cast<struct epoll_event*>(
                     malloc(_M_fdmap.size() * sizeof(struct epoll_event))
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

  struct epoll_event ev;

  if (type == fdtype::kFdSocket) {
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
  } else {
    ev.events = EPOLLIN;
  }

  ev.data.u64 = 0;
  ev.data.fd = fd;

  if (epoll_ctl(_M_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
    _M_fdmap.remove(fd);
    return false;
  }

  return true;
}

bool net::selector::remove(unsigned fd)
{
  if (!_M_fdmap.remove(fd)) {
    // The file descriptor has not been inserted.
    return true;
  }

  // The file descriptor will be removed automatically from epoll.
  close(fd);

  return true;
}

void net::selector::process_events()
{
  // For each event...
  for (unsigned i = 0; i < _M_nevents; i++) {
    unsigned fd = _M_events[i].data.fd;

    fdtype type;
    io::event_handler* handler;
    if (_M_fdmap.get(fd, type, handler)) {
      uint32_t ev = _M_events[i].events;

      // If an error happened but EPOLLIN / EPOLLOUT were not set...
      if ((ev & (EPOLLERR | EPOLLHUP)) && ((ev & (EPOLLIN | EPOLLOUT)) == 0)) {
        ev |= (EPOLLIN | EPOLLOUT);
      }

      io::event events = io::event::kNoEvent;

      if (ev & EPOLLIN) {
        events = static_cast<io::event>(
                   static_cast<unsigned>(events) |
                   static_cast<unsigned>(io::event::kRead)
                 );
      }

      if (ev & EPOLLOUT) {
        events = static_cast<io::event>(
                   static_cast<unsigned>(events) |
                   static_cast<unsigned>(io::event::kWrite)
                 );
      }

      if (!process_fd_events(fd, type, handler, events)) {
        remove(fd);
      }
    }
  }
}
