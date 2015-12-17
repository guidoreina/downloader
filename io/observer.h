#ifndef IO_OBSERVER_H
#define IO_OBSERVER_H

#include "io/event_handler.h"

namespace io {
  class observer {
    public:
      // On success.
      virtual void on_success(event_handler* handler) = 0;

      // On error.
      virtual void on_error(event_handler* handler) = 0;
  };
}

#endif // IO_OBSERVER_H
