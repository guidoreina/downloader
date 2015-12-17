#include "net/http/client.h"
#include "util/ctype.h"
#include "macros/macros.h"

const string::buffer* net::http::client::_M_user_agent = NULL;

bool net::http::client::init(request* req,
                             const char* filename,
                             off_t max_file_size)
{
  if (!_M_file.open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644)) {
    return false;
  }

  if ((_M_filename = strdup(filename)) == NULL) {
    _M_file.close();
    unlink(filename);

    return false;
  }

  _M_request = req;

  _M_max_file_size = max_file_size;

  return true;
}

bool net::http::client::init(request* req,
                             string::buffer* buf,
                             const char* filename)
{
  if (!_M_file.open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644)) {
    return false;
  }

  if ((_M_filename = strdup(filename)) == NULL) {
    _M_file.close();
    unlink(filename);

    return false;
  }

  _M_request = req;

  _M_buf = buf;

  return true;
}

bool net::http::client::init(request* req,
                             string::buffer* buf,
                             size_t max_buffer_size,
                             const char* filename,
                             off_t max_file_size)
{
  if (!_M_file.open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644)) {
    return false;
  }

  if ((_M_filename = strdup(filename)) == NULL) {
    _M_file.close();
    unlink(filename);

    return false;
  }

  _M_request = req;

  _M_buf = buf;
  _M_max_buffer_size = max_buffer_size;

  _M_max_file_size = max_file_size;

  return true;
}

void net::http::client::clear()
{
  tcp_connection::free();

  _M_request = NULL;

  _M_headers.clear();

  _M_buf = NULL;
  _M_max_buffer_size = kDefaultMaxBufferSize;

  if (_M_filename) {
    ::free(_M_filename);
    _M_filename = NULL;
  }

  _M_file.close();
  _M_max_file_size = kDefaultMaxFileSize;

  _M_reason_phrase_len = 0;

  _M_state = state::kConnecting;
}

io::event_handler::result net::http::client::run()
{
  io::event_handler::result res;
  const string::buffer* bufs[2];
  size_t count;
  size_t to_send;
  int err;

  do {
    switch (_M_state) {
      case state::kConnecting:
        // Get socket error.
        if ((!_M_socket.get_socket_error(err)) || (err != 0)) {
          return error();
        }

        _M_state = state::kConnected;

        // Fall through.
      case state::kConnected:
#if HAVE_SSL
        // HTTP?
        if (_M_request->uri().scheme().length() == 4) {
#endif // HAVE_SSL
          // Build headers.
          if (!build_headers()) {
            return error();
          }

          if (!_M_socket.set_tcp_no_delay(true)) {
            return error();
          }

          _M_state = state::kSendingRequest;
#if HAVE_SSL
        } else {
          // HTTPS.

          // Perform handshake.
          _M_ssl_socket.fd(_M_socket.fd());
          switch (res = handshake(ssl_socket::ssl_mode::kClientMode)) {
            case io::event_handler::result::kSuccess:
              // Enable SSL.
              ssl(true);

              // Build headers.
              if (!build_headers()) {
                return error();
              }

              _M_state = state::kSendingRequest;
              break;
            case io::event_handler::result::kError:
              return error();
            default:
              _M_state = state::kPerformingHandshake;
              return res;
          }
        }
#endif // HAVE_SSL

        break;
#if HAVE_SSL
      case state::kPerformingHandshake:
        // Enable SSL.
        ssl(true);

        // Build headers.
        if (!build_headers()) {
          return error();
        }

        _M_state = state::kSendingRequest;
#endif // HAVE_SSL

        // Fall through.
      case state::kSendingRequest:
        if (_M_iovcnt == 1) {
          to_send = _M_out.length();

          res = write();
        } else {
          to_send = _M_out.length() + _M_request->content_length();

          bufs[0] = &_M_out;
          bufs[1] = _M_request->buffer();

          res = writev(bufs, 2);
        }

        switch (res) {
          case io::event_handler::result::kSuccess:
            // If the whole headers have been sent...
            if (static_cast<size_t>(_M_outp) == to_send) {
              _M_state = _M_next_state;
              _M_substate = 0;

              _M_outp = 0;
            } else {
              return io::event_handler::result::kSuccess;
            }

            break;
          case io::event_handler::result::kError:
            return error();
          default:
            return res;
        }

        break;
      case state::kSendingMessageBody:
        switch (res = sendfile(_M_request->file(),
                               _M_request->content_length())) {
          case io::event_handler::result::kSuccess:
            if (_M_outp == _M_request->content_length()) {
              if (!ssl()) {
                _M_socket.uncork();
              }

              _M_state = state::kReadingStatusLine;
            } else {
              return io::event_handler::result::kSuccess;
            }

            break;
          case io::event_handler::result::kError:
            return error();
          default:
            return res;
        }

        // Fall through.
      case state::kReadingStatusLine:
        // If the input buffer is empty...
        if ((count = _M_in.length() - _M_inp) == 0) {
          if (!_M_readable) {
            return io::event_handler::result::kChangeToReadMode;
          }

          // Read status-line.
          res = read(count);
        }

        if (count > 0) {
          // Parse Status-Line.
          switch (parse_status_line()) {
            case parse_result::kEndOfData:
              _M_headers.clear();

              _M_state = state::kReadingHeaders;
              break;
            case parse_result::kInvalidData:
              return error();
            case parse_result::kNotEndOfData:
              break;
          }
        } else {
          switch (res) {
            case io::event_handler::result::kSuccess:
              break;
            case io::event_handler::result::kError:
              return error();
            default:
              return res;
          }
        }

        break;
      case state::kReadingHeaders:
        // If the input buffer is empty...
        if ((count = _M_in.length() - _M_inp) == 0) {
          if (!_M_readable) {
            return io::event_handler::result::kSuccess;
          }

          // Read headers.
          res = read(count);
        }

        if (count > 0) {
          // Parse headers.
          switch (_M_headers.parse(_M_in.data() + _M_inp,
                                   _M_in.length() - _M_inp)) {
            case header::headers::parse_result::kEndOfHeader:
              _M_state = state::kProcessingHeaders;
              break;
            case header::headers::parse_result::kNotEndOfHeader:
              break;
            default:
              return error();
          }
        } else {
          switch (res) {
            case io::event_handler::result::kSuccess:
              break;
            case io::event_handler::result::kError:
              return error();
            default:
              return res;
          }
        }

        break;
      case state::kProcessingHeaders:
        // Skip headers.
        _M_inp += _M_headers.size();

        {
          string::slice uri(_M_request->uri().string());

          // Save URI and headers.
          if ((!add_data(uri.data(), uri.length())) ||
              (!add_data("\r\n", 2)) ||
              (!add_data(_M_in.data(), _M_inp))) {
            return error();
          }
        }

        if (_M_request->method() != method::kHead) {
          // RFC 2616: 4.4 Message length.

          // Get Transfer-Encoding header (if available).
          string::slice encoding =
            _M_headers.header(header::permanent_field_name::kTransferEncoding);

          // If not chunked...
          if ((encoding.length() != 7) ||
              (strncasecmp(encoding.data(), "chunked", 7) != 0)) {
            // Get Content-Length header (if available).
            if (_M_headers.header(header::permanent_field_name::kContentLength,
                                  _M_content_length)) {
              if (_M_content_length == 0) {
                return finished();
              }

              _M_received = 0;

              _M_state = state::kHaveContentLength;
            } else {
              // Get Connection header (if available).
              string::slice connection =
                _M_headers.header(header::permanent_field_name::kConnection);

              // Keep-Alive?
              if ((connection.length() == 10) &&
                  (strncasecmp(connection.data(), "Keep-Alive", 10) == 0)) {
                // Content-Length is needed.
                return error();
              } else if (((_M_major_number == 1) && (_M_minor_number == 0)) ||
                         ((connection.length() == 5) &&
                          (strncasecmp(connection.data(), "close", 5) == 0))) {
                _M_content_length = 0;

                _M_state = state::kDontHaveContentLength;
              } else {
                // Content-Length is needed.
                return error();
              }
            }
          } else {
            // Chunked.

            _M_chunk_size = 0;
            _M_substate = 0;

            _M_state = state::kChunkedTransferEncoding;
          }
        } else {
          // HEAD method.
          return finished();
        }

        break;
      case state::kHaveContentLength:
        // If the input buffer is empty...
        if ((count = _M_in.length() - _M_inp) == 0) {
          if (!_M_readable) {
            return io::event_handler::result::kSuccess;
          }

          // Read message body.
          res = read(count);
        }

        if (count > 0) {
          size_t left = _M_content_length - _M_received;

          if (count >= left) {
            if (!add_data(_M_in.data() + _M_inp, left)) {
              return error();
            }

            return finished();
          } else {
            if (!add_data(_M_in.data() + _M_inp, count)) {
              return error();
            }

            _M_received += count;

            _M_in.clear();
            _M_inp = 0;
          }
        } else {
          switch (res) {
            case io::event_handler::result::kSuccess:
              break;
            case io::event_handler::result::kError:
              return error();
            default:
              return res;
          }
        }

        break;
      case state::kDontHaveContentLength:
        // If the input buffer is empty...
        if ((count = _M_in.length() - _M_inp) == 0) {
          if (!_M_readable) {
            return io::event_handler::result::kSuccess;
          }

          // Read message body.
          res = read(count);
        }

        if (count > 0) {
          if (!add_data(_M_in.data() + _M_inp, count)) {
            return error();
          }

          _M_content_length += count;

          _M_in.clear();
          _M_inp = 0;
        } else {
          switch (res) {
            case io::event_handler::result::kSuccess:
              break;
            case io::event_handler::result::kError:
              return finished();
            default:
              return res;
          }
        }

        break;
      case state::kChunkedTransferEncoding:
        // If the input buffer is empty...
        if ((count = _M_in.length() - _M_inp) == 0) {
          if (!_M_readable) {
            return io::event_handler::result::kSuccess;
          }

          // Read message body.
          res = read(count);
        }

        if (count > 0) {
          // Parse chunked body.
          switch (parse_chunked_body()) {
            case parse_result::kEndOfData:
              return finished();
            case parse_result::kInvalidData:
              return error();
            case parse_result::kNotEndOfData:
              break;
          }
        } else {
          switch (res) {
            case io::event_handler::result::kSuccess:
              break;
            case io::event_handler::result::kError:
              return error();
            default:
              return res;
          }
        }

        break;
      case state::kFinished: // Never reached.
        break;
    }
  } while (true);
}

bool net::http::client::build_headers()
{
  string::slice m(methods::name(_M_request->method()));
  const string::slice& path(_M_request->uri().path());

  if (!_M_out.format("%.*s %.*s HTTP/1.1\r\n",
      m.length(),
      m.data(),
      path.length(),
      path.data())) {
    return false;
  }

  if (!_M_headers.add(header::permanent_field_name::kConnection,
                      "close",
                      5)) {
    return false;
  }

  const string::slice& host(_M_request->uri().host());
  if (!_M_headers.add(header::permanent_field_name::kHost,
                      host.data(),
                      host.length())) {
    return false;
  }

  switch (_M_request->method()) {
    case method::kPost:
    case method::kPut:
      if (!_M_headers.add(header::permanent_field_name::kContentLength,
                          _M_request->content_length())) {
        return false;
      }

      if (_M_request->content_length() > 0) {
        if (_M_request->buffer()) {
          _M_iovcnt = 2;
          _M_next_state = state::kReadingStatusLine;
        } else {
          _M_iovcnt = 1;
          _M_next_state = state::kSendingMessageBody;

          if (!ssl()) {
            _M_socket.cork();
          }
        }
      } else {
        _M_iovcnt = 1;
        _M_next_state = state::kReadingStatusLine;
      }

      break;
    default:
      _M_iovcnt = 1;
      _M_next_state = state::kReadingStatusLine;
  }

  if (_M_user_agent) {
    if (!_M_headers.add(header::permanent_field_name::kUserAgent,
                        _M_user_agent->data(),
                        _M_user_agent->length())) {
      return false;
    }
  }

  return _M_headers.serialize(_M_out);
}

net::http::client::parse_result net::http::client::parse_status_line()
{
  const uint8_t* data = reinterpret_cast<const uint8_t*>(_M_in.data());
  size_t len = _M_in.length();

  // Format:
  // HTTP/<major-version>.<minor-version> <status-code> [<reason-phrase>]

  while (static_cast<size_t>(_M_inp) < len) {
    uint8_t c = data[_M_inp];

    switch (_M_substate) {
      case 0: // [H]TTP/<major-version>.<minor-version> ...
        if ((c != 'H') && (c != 'h')) {
          return parse_result::kInvalidData;
        }

        _M_substate = 1; // H[T]TP/<major-version>.<minor-version> ...
        break;
      case 1: // H[T]TP/<major-version>.<minor-version> ...
        if ((c != 'T') && (c != 't')) {
          return parse_result::kInvalidData;
        }

        _M_substate = 2; // HT[T]P/<major-version>.<minor-version> ...
        break;
      case 2: // HT[T]P/<major-version>.<minor-version> ...
        if ((c != 'T') && (c != 't')) {
          return parse_result::kInvalidData;
        }

        _M_substate = 3; // HTT[P]/<major-version>.<minor-version> ...
        break;
      case 3: // HTT[P]/<major-version>.<minor-version> ...
        if ((c != 'P') && (c != 'p')) {
          return parse_result::kInvalidData;
        }

        _M_substate = 4; // HTTP[/]<major-version>.<minor-version> ...
        break;
      case 4: // HTTP[/]<major-version>.<minor-version> ...
        if (c != '/') {
          return parse_result::kInvalidData;
        }

        _M_substate = 5; // HTTP/[<major-version>].<minor-version> ...
        break;
      case 5: // HTTP/[<major-version>].<minor-version> ...
        if (!util::is_digit(c)) {
          return parse_result::kInvalidData;
        }

        if ((_M_major_number = c - '0') > 1) {
          return parse_result::kInvalidData;
        }

        _M_substate = 6; // Parsing major-version.
        break;
      case 6: // Parsing major-version.
        if (c == '.') {
          _M_substate = 7; // Dot between major-version and minor-version.
        } else if (util::is_digit(c)) {
          if ((_M_major_number = (_M_major_number * 10) + (c - '0')) > 1) {
            return parse_result::kInvalidData;
          }
        } else {
          return parse_result::kInvalidData;
        }

        break;
      case 7: // Dot between major-version and minor-version.
        if (!util::is_digit(c)) {
          return parse_result::kInvalidData;
        }

        if ((_M_major_number == 1) && ((_M_minor_number = c - '0') > 1)) {
          return parse_result::kInvalidData;
        }

        _M_substate = 8; // Parsing minor-version.
        break;
      case 8: // Parsing minor-version.
        if (util::is_white_space(c)) {
          _M_substate = 9; // White space after HTTP-version.
        } else if (util::is_digit(c)) {
          unsigned tmp;
          if ((tmp = (_M_minor_number * 10) + (c - '0')) < _M_minor_number) {
            return parse_result::kInvalidData;
          }

          if ((_M_major_number == 1) && (tmp > 1)) {
            return parse_result::kInvalidData;
          }

          _M_minor_number = tmp;
        } else {
          return parse_result::kInvalidData;
        }

        break;
      case 9: // White space after HTTP-version.
        if (util::is_digit(c)) {
          _M_status_code = c - '0';

          _M_substate = 10; // Parsing status-code.
        } else if (!util::is_white_space(c)) {
          return parse_result::kInvalidData;
        }

        break;
      case 10: // Parsing status-code.
        if (util::is_digit(c)) {
          if ((_M_status_code = (_M_status_code * 10) + (c - '0')) > 599) {
            return parse_result::kInvalidData;
          }
        } else if (util::is_white_space(c)) {
          if (_M_status_code < 100) {
            return parse_result::kInvalidData;
          }

          _M_substate = 11; // White space after status-code.
        } else if (c == '\r') {
          if (_M_status_code < 100) {
            return parse_result::kInvalidData;
          }

          _M_substate = 13; // '\r' at the end of the Status-Line.
        } else if (c == '\n') {
          if (_M_status_code < 100) {
            return parse_result::kInvalidData;
          }

          _M_inp++;

          return parse_result::kEndOfData;
        } else {
          return parse_result::kInvalidData;
        }

        break;
      case 11: // White space after status-code.
        if (c > ' ') {
          _M_reason_phrase = _M_inp;

          _M_substate = 12; // Parsing reason-phrase.
        } else if (c == '\r') {
          _M_substate = 13; // '\r' at the end of the Status-Line.
        } else if (c == '\n') {
          _M_inp++;

          return parse_result::kEndOfData;
        } else if (!util::is_white_space(c)) {
          return parse_result::kInvalidData;
        }

        break;
      case 12: // Parsing reason-phrase.
        switch (c) {
          case '\r':
            _M_reason_phrase_len = _M_inp - _M_reason_phrase;

            _M_substate = 13; // '\r' at the end of the Status-Line.
            break;
          case '\n':
            _M_reason_phrase_len = _M_inp++ - _M_reason_phrase;

            return parse_result::kEndOfData;
          default:
            if ((c < ' ') && (c != '\t')) {
              return parse_result::kInvalidData;
            }
        }

        break;
      case 13: // '\r' at the end of the Status-Line.
        if (c != '\n') {
          return parse_result::kInvalidData;
        }

        _M_inp++;

        return parse_result::kEndOfData;
    }

    if (static_cast<size_t>(++_M_inp) == kStatusLineMaxLen) {
      return parse_result::kInvalidData;
    }
  }

  return parse_result::kNotEndOfData;
}

net::http::client::parse_result net::http::client::parse_chunked_body()
{
  const uint8_t* data = reinterpret_cast<const uint8_t*>(_M_in.data());
  size_t len = _M_in.length();

  while (static_cast<size_t>(_M_inp) < len) {
    uint8_t c = data[_M_inp];

    switch (_M_substate) {
      case 0: // Before chunk size.
        {
          int digit;
          if ((digit = util::hex2dec(c)) < 0) {
            return parse_result::kInvalidData;
          }

          _M_chunk_size = digit;

          _M_substate = 1; // Reading chunk size.
        }

        _M_inp++;
        break;
      case 1: // Reading chunk size.
        switch (c) {
          case '\r':
            _M_substate = 3; // '\r' in chunk size line.
            break;
          case '\n':
            if (_M_chunk_size > 0) {
              _M_substate = 4; // '\n' in chunk size line.
            } else {
              _M_chunk_trailer_len = 0;

              _M_substate = 8; // After last chunk.
            }

            break;
          case ' ':
          case '\t':
          case ';':
            _M_chunk_extension_len = 0;

            _M_substate = 2; // Chunk extension.
            break;
          default:
            {
              int digit;
              if ((digit = util::hex2dec(c)) < 0) {
                return parse_result::kInvalidData;
              }

              size_t tmp;
              if ((tmp = (_M_chunk_size * 16) + digit) < _M_chunk_size) {
                return parse_result::kInvalidData;
              }

              _M_chunk_size = tmp;
            }
        }

        _M_inp++;
        break;
      case 2: // Chunk extension.
        switch (c) {
          case '\r':
            _M_substate = 3; // '\r' in chunk size line.
            break;
          case '\n':
            if (_M_chunk_size > 0) {
              _M_substate = 4; // '\n' in chunk size line.
            } else {
              _M_chunk_trailer_len = 0;

              _M_substate = 8; // After last chunk.
            }

            break;
          default:
            if ((++_M_chunk_extension_len > kChunkExtensionMaxLen) ||
                ((c < ' ') && (c != '\t'))) {
              return parse_result::kInvalidData;
            }
        }

        _M_inp++;
        break;
      case 3: // '\r' in chunk size line.
        if (c != '\n') {
          return parse_result::kInvalidData;
        }

        if (_M_chunk_size > 0) {
          _M_substate = 4; // '\n' in chunk size line.
        } else {
          _M_chunk_trailer_len = 0;

          _M_substate = 8; // After last chunk.
        }

        _M_inp++;
        break;
      case 4: // '\n' in chunk size line.
        _M_received = len - _M_inp;

        if (_M_received < _M_chunk_size) {
          if (!add_data(data + _M_inp, _M_received)) {
            return parse_result::kInvalidData;
          }

          _M_in.clear();
          _M_inp = 0;

          _M_substate = 5; // Reading chunk data.

          return parse_result::kNotEndOfData;
        } else {
          if (!add_data(data + _M_inp, _M_chunk_size)) {
            return parse_result::kInvalidData;
          }

          _M_inp += _M_chunk_size;

          _M_substate = 6; // After chunk data.
        }

        break;
      case 5: // Reading chunk data.
        {
          size_t left = _M_chunk_size - _M_received;

          if (len < left) {
            if (!add_data(data, len)) {
              return parse_result::kInvalidData;
            }

            _M_received += len;

            _M_in.clear();

            return parse_result::kNotEndOfData;
          } else {
            if (!add_data(data, left)) {
              return parse_result::kInvalidData;
            }

            _M_inp = left;

            _M_substate = 6; // After chunk data.
          }
        }

        break;
      case 6: // After chunk data.
        switch (c) {
          case '\r':
            _M_substate = 7; // '\r' after chunk data.
            break;
          case '\n':
            _M_substate = 0; // Before chunk size.
            break;
          default:
            return parse_result::kInvalidData;
        }

        _M_inp++;
        break;
      case 7: // '\r' after chunk data.
        if (c != '\n') {
          return parse_result::kInvalidData;
        }

        _M_substate = 0; // Before chunk size.

        _M_inp++;
        break;
      case 8: // After last chunk.
        switch (c) {
          case '\r':
            _M_substate = 11; // '\r' after last chunk.
            break;
          case '\n':
            _M_inp++;

            return parse_result::kEndOfData;
          default:
            if ((++_M_chunk_trailer_len > kChunkTrailerMaxLen) || (c <= ' ')) {
              return parse_result::kInvalidData;
            }

            _M_substate = 9; // Trailer.
        }

        _M_inp++;
        break;
      case 9: // Trailer.
        if (++_M_chunk_trailer_len > kChunkTrailerMaxLen) {
          return parse_result::kInvalidData;
        }

        switch (c) {
          case '\r':
            _M_substate = 10; // '\r' after trailer line.
            break;
          case '\n':
            _M_substate = 8; // After last chunk.
            break;
          default:
            if ((c < ' ') && (c != '\t')) {
              return parse_result::kInvalidData;
            }
        }

        _M_inp++;
        break;
      case 10: // '\r' after trailer line.
        if ((++_M_chunk_trailer_len > kChunkTrailerMaxLen) || (c != '\n')) {
          return parse_result::kInvalidData;
        }

        _M_substate = 8; // After last chunk.

        _M_inp++;
        break;
      case 11: // '\r' after last chunk.
        if (c != '\n') {
          return parse_result::kInvalidData;
        }

        _M_inp++;

        return parse_result::kEndOfData;
    }
  }

  return parse_result::kNotEndOfData;
}
