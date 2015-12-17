#ifndef TIMER_OBSERVER_H
#define TIMER_OBSERVER_H

#include "timer/event_handler.h"

namespace timer {
  class observer {
    public:
      // On timer.
      virtual void on_timer(event_handler* handler) = 0;
  };
}

#endif // TIMER_OBSERVER_H
