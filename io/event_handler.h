#ifndef IO_EVENT_HANDLER_H
#define IO_EVENT_HANDLER_H

#include "io/event.h"

namespace io {
  class event_handler {
    public:
      enum class result {
        kError,
        kChangeToReadMode,
        kChangeToWriteMode,
        kSuccess
      };

      // On I/O.
      virtual result on_io(event events) = 0;
  };
}

#endif // IO_EVENT_HANDLER_H
