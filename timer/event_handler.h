#ifndef TIMER_EVENT_HANDLER_H
#define TIMER_EVENT_HANDLER_H

namespace timer {
  class event_handler {
    public:
      // On timer.
      virtual bool on_timer() = 0;
  };
}

#endif // TIMER_EVENT_HANDLER_H
