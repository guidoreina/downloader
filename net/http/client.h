#ifndef NET_HTTP_CLIENT_H
#define NET_HTTP_CLIENT_H

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "net/tcp_connection.h"
#include "net/http/request.h"
#include "net/http/header/headers.h"

namespace net {
  namespace http {
    class client : public tcp_connection {
      public:
        static const size_t kDefaultMaxBufferSize = 32 * 1024;
        static const off_t kDefaultMaxFileSize = 0; // No limit.

        // Constructor.
        client();

        // Destructor.
        ~client();

        // Initialize.
        void init(request* req,
                  string::buffer* buf,
                  size_t max_buffer_size = kDefaultMaxBufferSize);

        bool init(request* req,
                  const char* filename,
                  off_t max_file_size = kDefaultMaxFileSize);

        bool init(request* req, string::buffer* buf, const char* filename);

        bool init(request* req,
                  string::buffer* buf,
                  size_t max_buffer_size,
                  const char* filename,
                  off_t max_file_size);

        // Clear.
        void clear();

        // Set User-Agent.
        static void user_agent(const string::buffer* user_agent);

        // Run.
        io::event_handler::result run();

      protected:
        // On timer.
        bool on_timer();

      private:
        static const size_t kStatusLineMaxLen = 128;
        static const size_t kChunkExtensionMaxLen = 1024;
        static const size_t kChunkTrailerMaxLen = 1024;

        request* _M_request;

        header::headers _M_headers;

        static const string::buffer* _M_user_agent;

        string::buffer* _M_buf;
        size_t _M_max_buffer_size;

        char* _M_filename;
        fs::file _M_file;
        off_t _M_max_file_size;

        unsigned _M_iovcnt;

        unsigned _M_major_number;
        unsigned _M_minor_number;

        unsigned _M_status_code;

        size_t _M_reason_phrase;
        size_t _M_reason_phrase_len;

        uint64_t _M_content_length;
        uint64_t _M_received;

        size_t _M_chunk_size;
        size_t _M_chunk_extension_len;
        size_t _M_chunk_trailer_len;

        enum class state : uint8_t {
          kConnecting,
          kConnected,

#if HAVE_SSL
          kPerformingHandshake,
#endif

          kSendingRequest,
          kSendingMessageBody,
          kReadingStatusLine,
          kReadingHeaders,
          kProcessingHeaders,
          kHaveContentLength,
          kDontHaveContentLength,
          kChunkedTransferEncoding,
          kFinished
        };

        state _M_state;
        state _M_next_state;

        uint8_t _M_substate;

        // Build headers.
        bool build_headers();

        enum class parse_result {
          kInvalidData,
          kNotEndOfData,
          kEndOfData
        };

        // Parse Status-Line.
        parse_result parse_status_line();

        // Parse chunked body.
        parse_result parse_chunked_body();

        // Add data.
        bool add_data(const void* data, size_t len);

        // On error.
        void on_error();

        io::event_handler::result finished();
        io::event_handler::result error();

        // Disable copy constructor and assignment operator.
        client(const client&) = delete;
        client& operator=(const client&) = delete;
    };

    inline client::client()
      : _M_request(NULL),
        _M_buf(NULL),
        _M_max_buffer_size(kDefaultMaxBufferSize),
        _M_filename(NULL),
        _M_max_file_size(kDefaultMaxFileSize),
        _M_reason_phrase_len(0),
        _M_state(state::kConnecting)
    {
    }

    inline client::~client()
    {
      if (_M_filename) {
        ::free(_M_filename);
      }
    }

    inline void client::init(request* req,
                             string::buffer* buf,
                             size_t max_buffer_size)
    {
      _M_request = req;
      _M_buf = buf;
      _M_max_buffer_size = max_buffer_size;
    }

    inline void client::user_agent(const string::buffer* user_agent)
    {
      _M_user_agent = user_agent;
    }

    inline bool client::on_timer()
    {
      on_error();
      return false;
    }

    inline bool client::add_data(const void* data, size_t len)
    {
      if ((_M_buf) &&
          (!_M_buf->append(reinterpret_cast<const char*>(data), len))) {
        return false;
      }

      if ((_M_filename) &&
          (_M_file.write(data, len) != static_cast<ssize_t>(len))) {
        return false;
      }

      return true;
    }

    inline void client::on_error()
    {
      if (_M_filename) {
        _M_file.close();
        unlink(_M_filename);

        ::free(_M_filename);
        _M_filename = NULL;
      }
    }

    inline io::event_handler::result client::finished()
    {
      if (_M_filename) {
        _M_file.close();
      }

      _M_state = state::kFinished;

      return io::event_handler::result::kError;
    }

    inline io::event_handler::result client::error()
    {
      on_error();
      return io::event_handler::result::kError;
    }
  }
}

#endif // NET_HTTP_CLIENT_H
