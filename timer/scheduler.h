#ifndef TIMER_SCHEDULER_H
#define TIMER_SCHEDULER_H

#include <stdlib.h>
#include "timer/timer.h"
#include "timer/observer.h"
#include "timer/priority.h"

namespace timer {
  template<priority_t kNumberPriorities = 2>
  class scheduler {
    public:
      // Constructor.
      scheduler();

      // Clear scheduler.
      void clear();

      // Set observer.
      void set_observer(observer* obs);

      // Schedule timer.
      bool schedule(priority_t priority, timer* t, uint64_t expiration_time);

      // Reschedule timer.
      bool reschedule(priority_t priority, timer* t, uint64_t expiration_time);

      // Erase timer.
      void erase(timer* t);

      // Check expired timers.
      void check_expired(uint64_t current_msec);

    private:
      timer _M_timers[kNumberPriorities];

      observer* _M_observer;

      // Handle expired timer.
      void handle_expired(event_handler* handler);

      // Disable copy constructor and assignment operator.
      scheduler(const scheduler&) = delete;
      scheduler& operator=(const scheduler&) = delete;
  };

  template<priority_t kNumberPriorities>
  inline scheduler<kNumberPriorities>::scheduler()
    : _M_observer(NULL)
  {
    clear();
  }

  template<priority_t kNumberPriorities>
  inline void scheduler<kNumberPriorities>::clear()
  {
    for (size_t i = 0; i < kNumberPriorities; i++) {
      _M_timers[i]._M_prev = &_M_timers[i];
      _M_timers[i]._M_next = &_M_timers[i];
    }
  }

  template<priority_t kNumberPriorities>
  inline void scheduler<kNumberPriorities>::set_observer(observer* obs)
  {
    _M_observer = obs;
  }

  template<priority_t kNumberPriorities>
  bool scheduler<kNumberPriorities>::schedule(priority_t priority,
                                              timer* t,
                                              uint64_t expiration_time)
  {
    if (priority >= kNumberPriorities) {
      return false;
    }

    t->_M_expiration_time = expiration_time;

    // Insert at the end.
    t->_M_prev = _M_timers[priority]._M_prev;
    t->_M_next = &_M_timers[priority];

    _M_timers[priority]._M_prev->_M_next = t;
    _M_timers[priority]._M_prev = t;

    return true;
  }

  template<priority_t kNumberPriorities>
  inline bool scheduler<kNumberPriorities>::reschedule(priority_t priority,
                                                       timer* t,
                                                       uint64_t expiration_time)
  {
    if (priority >= kNumberPriorities) {
      return false;
    }

    t->_M_expiration_time = expiration_time;

    erase(t);
    schedule(priority, t, expiration_time);

    return true;
  }

  template<priority_t kNumberPriorities>
  inline void scheduler<kNumberPriorities>::erase(timer* t)
  {
    t->_M_prev->_M_next = t->_M_next;
    t->_M_next->_M_prev = t->_M_prev;
  }

  template<priority_t kNumberPriorities>
  inline void scheduler<kNumberPriorities>::check_expired(uint64_t current_msec)
  {
    for (size_t i = 0; i < kNumberPriorities; i++) {
      timer* t;
      while (((t = _M_timers[i]._M_next) != &_M_timers[i]) &&
             (t->_M_expiration_time <= current_msec)) {
        handle_expired(t->_M_handler);

        erase(t);
      }
    }
  }

  template<priority_t kNumberPriorities>
  inline
  void scheduler<kNumberPriorities>::handle_expired(event_handler* handler)
  {
    handler->on_timer();

    if (_M_observer) {
      _M_observer->on_timer(handler);
    }
  }
}

#endif // TIMER_SCHEDULER_H
