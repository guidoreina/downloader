#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H

#include <stdint.h>
#include "timer/event_handler.h"
#include "timer/priority.h"

namespace timer {
  class timer {
    template<priority_t kNumberPriorities>
    friend class scheduler;

    public:
      // Constructor.
      timer(event_handler* handler = NULL);

    private:
      event_handler* _M_handler;
      uint64_t _M_expiration_time;

      timer* _M_prev;
      timer* _M_next;
  };

  inline timer::timer(event_handler* handler)
    : _M_handler(handler)
  {
  }
}

#endif // TIMER_TIMER_H
