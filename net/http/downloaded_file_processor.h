#ifndef NET_HTTP_DOWNLOADED_FILE_PROCESSOR_H
#define NET_HTTP_DOWNLOADED_FILE_PROCESSOR_H

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include "net/http/header/headers.h"
#include "net/uri/uri.h"
#include "string/buffer.h"
#include "string/slice.h"

namespace net {
  namespace http {
    class downloaded_file_processor {
      public:
        // Constructor.
        downloaded_file_processor();

        // Destructor.
        ~downloaded_file_processor();

        // Open file.
        bool open(const char* filename);

        // Get URI.
        const string::slice& uri() const;

        // Get status-code.
        unsigned status_code() const;

        // Get headers.
        const header::headers& headers() const;

        // Get Content-Type.
        enum class content_type {
          kTextPlain,
          kTextHtml,
          kOther
        };

        content_type get_content_type() const;

        // Read title.
        bool read_title(string::slice& title);

        // Configuration.
        struct configuration {
          const char* urls_begin;
          const char* urls_end;

          const char* url_separators;

          // Constructor.
          configuration();
        };

        // Configure.
        void configure(configuration& config);

        // Get next URI.
        bool next(uri::uri& uri);

      private:
        int _M_fd;
        uint8_t* _M_data;
        uint64_t _M_len;

        const uint8_t* _M_end;
        const uint8_t* _M_body;
        const uint8_t* _M_ptr;

        configuration _M_config;

        string::slice _M_uri;

        unsigned _M_status_code;

        header::headers _M_headers;

        content_type _M_content_type;

        string::buffer _M_buf;

        // Read status code.
        bool read_status_code();

        // Read headers.
        bool read_headers();

        // Disable copy constructor and assignment operator.
        downloaded_file_processor(const downloaded_file_processor&) = delete;

        downloaded_file_processor&
        operator=(const downloaded_file_processor&) = delete;
    };

    inline downloaded_file_processor::downloaded_file_processor()
      : _M_fd(-1),
        _M_data(reinterpret_cast<uint8_t*>(MAP_FAILED))
    {
    }

    inline downloaded_file_processor::~downloaded_file_processor()
    {
      if (_M_data != MAP_FAILED) {
        munmap(_M_data, _M_len);
      }

      if (_M_fd != -1) {
        close(_M_fd);
      }
    }

    inline const string::slice& downloaded_file_processor::uri() const
    {
      return _M_uri;
    }

    inline unsigned downloaded_file_processor::status_code() const
    {
      return _M_status_code;
    }

    inline const header::headers& downloaded_file_processor::headers() const
    {
      return _M_headers;
    }

    inline downloaded_file_processor::content_type
    downloaded_file_processor::get_content_type() const
    {
      return _M_content_type;
    }

    inline downloaded_file_processor::configuration::configuration()
      : urls_begin(NULL),
        urls_end(NULL),
        url_separators(" \"'\t\r\n")
    {
    }

    inline void downloaded_file_processor::configure(configuration& config)
    {
      _M_config = config;
    }
  }
}

#endif // NET_HTTP_DOWNLOADED_FILE_PROCESSOR_H
