#ifndef NET_SELECTOR_H
#define NET_SELECTOR_H

#if HAVE_EPOLL
  #include "net/epoll_selector.h"
#elif HAVE_KQUEUE
  #include "net/kqueue_selector.h"
#elif HAVE_PORT
  #include "net/port_selector.h"
#elif HAVE_POLL
  #include "net/poll_selector.h"
#else
  #include "net/select_selector.h"
#endif

#endif // NET_SELECTOR_H
