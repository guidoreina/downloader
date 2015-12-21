#include <unistd.h>
#include "net/kqueue_selector.h"

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

  if ((_M_fd = kqueue()) < 0) {
    return false;
  }

  if ((_M_events = reinterpret_cast<struct kevent*>(
                     malloc(_M_fdmap.size() * sizeof(struct kevent))
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

  struct kevent ev[2];
  unsigned nevents;

  if (type == fdtype::kFdSocket) {
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    EV_SET(&ev[0], fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
    EV_SET(&ev[1], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
#elif defined(__NetBSD__) || defined(__minix)
    EV_SET(&ev[0], fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, 0);
    EV_SET(&ev[1], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, 0);
#endif

    nevents = 2;
  } else {
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    EV_SET(&ev[0], fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
#elif defined(__NetBSD__) || defined(__minix)
    EV_SET(&ev[0], fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, 0);
#endif

    nevents = 1;
  }

  struct timespec timeout = {0, 0};

  if (kevent(_M_fd, ev, nevents, NULL, 0, &timeout) < 0) {
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

  // Events which are attached to file descriptors are automatically deleted
  // on the last close of the descriptor.
  close(fd);

  return true;
}

void net::selector::process_events()
{
  // For each event...
  for (unsigned i = 0; i < _M_nevents; i++) {
    if (_M_events[i].flags & EV_ERROR) {
      continue;
    }

    unsigned fd = static_cast<unsigned>(_M_events[i].ident);

    fdtype type;
    io::event_handler* handler;
    if (_M_fdmap.get(fd, type, handler)) {
      if (_M_events[i].filter == EVFILT_READ) {
        if (!process_fd_events(fd, type, handler, io::event::kRead)) {
          remove(fd);
        }
      } else if (_M_events[i].filter == EVFILT_WRITE) {
        if (!process_fd_events(fd, type, handler, io::event::kWrite)) {
          remove(fd);
        }
      }
    }
  }
}
