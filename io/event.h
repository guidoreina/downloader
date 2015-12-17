#ifndef IO_EVENT_H
#define IO_EVENT_H

namespace io {
  enum class event {
    kNoEvent = 0,
    kRead = 1,
    kWrite = 2
  };
}

#endif // IO_EVENT_H
