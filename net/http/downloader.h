#ifndef NET_HTTP_DOWNLOADER_H
#define NET_HTTP_DOWNLOADER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "net/selector.h"
#include "net/http/client.h"
#include "net/http/request.h"
#include "timer/scheduler.h"
#include "timer/observer.h"
#include "io/observer.h"

namespace net {
  namespace http {
    class downloader : public io::observer, public timer::observer {
      public:
        static const size_t kMinConnections = 1;
        static const size_t kMaxConnections = 2048;
        static const size_t kDefaultConnections = 100;

        static const char* kDefaultUrlsFile;
        static const char* kDefaultDirectory;

        // Constructor.
        downloader();

        // Destructor.
        ~downloader();

        // Create.
        bool create(const char* url_file, const char* dir);

        // Start.
        void start();

        // Stop.
        void stop();

        // Set maximum number of simultaneous connections.
        void max_connections(size_t value);

        // Set User-Agent.
        bool user_agent(const char* user_agent);

        // On I/O success.
        void on_success(io::event_handler* handler);

        // On I/O error.
        void on_error(io::event_handler* handler);

        // On timer.
        void on_timer(timer::event_handler* handler);

      private:
        static const timer::priority_t kNormalPriority = 1;
        static const unsigned kClientTimeout = 30; // Seconds.

        selector _M_selector;
        timer::scheduler<2> _M_scheduler;

        client* _M_clients;
        request* _M_requests;

        char _M_url_file[PATH_MAX];
        char _M_dir[PATH_MAX];

        FILE* _M_file;
        size_t _M_nfiles;

        size_t _M_max_connections;

        string::buffer _M_user_agent;

        size_t _M_count;

        time_t _M_current_time;
        uint64_t _M_current_msec;
        struct tm _M_localtime;

        bool _M_running;

        // Update time.
        void update_time();

        // Load URLs.
        bool load_urls();

        // Resolve.
        static bool resolve(const char* host, socket_address& addr);

        // Disable copy constructor and assignment operator.
        downloader(const downloader&) = delete;
        downloader& operator=(const downloader&) = delete;
    };

    inline downloader::downloader()
      : _M_clients(NULL),
        _M_requests(NULL),
        _M_file(NULL),
        _M_nfiles(0),
        _M_max_connections(kDefaultConnections),
        _M_count(0),
        _M_running(false)
    {
      _M_selector.set_io_observer(this);
      _M_scheduler.set_observer(this);

      update_time();
    }

    inline downloader::~downloader()
    {
      if (_M_clients) {
        delete [] _M_clients;
      }

      if (_M_requests) {
        delete [] _M_requests;
      }

      if (_M_file) {
        fclose(_M_file);
      }
    }

    inline void downloader::stop()
    {
      _M_running = false;
    }

    inline void downloader::max_connections(size_t value)
    {
      _M_max_connections = value;
    }

    inline bool downloader::user_agent(const char* user_agent)
    {
      _M_user_agent.clear();

      if (!_M_user_agent.append(user_agent)) {
        return false;
      }

      client::user_agent(&_M_user_agent);

      return true;
    }

    inline void downloader::on_success(io::event_handler* handler)
    {
      _M_scheduler.reschedule(kNormalPriority,
                              static_cast<client*>(handler)->timer(),
                              _M_current_msec + (kClientTimeout * 1000));
    }

    inline void downloader::on_error(io::event_handler* handler)
    {
      _M_scheduler.erase(static_cast<client*>(handler)->timer());
    }

    inline void downloader::on_timer(timer::event_handler* handler)
    {
      _M_selector.remove(static_cast<client*>(handler)->fd());
    }

    inline void downloader::update_time()
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);

      _M_current_time = tv.tv_sec;
      _M_current_msec = (static_cast<uint64_t>(_M_current_time) * 1000) +
                        (tv.tv_usec / 1000);

      localtime_r(&_M_current_time, &_M_localtime);
    }
  }
}

#endif // NET_HTTP_DOWNLOADER_H
