#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <new>
#include "net/http/downloader.h"
#include "net/http/methods.h"
#include "net/uri/uri.h"
#include "net/ipv4_address.h"
#include "net/ipv6_address.h"
#include "net/ports.h"
#include "string/slice.h"
#include "util/move.h"

const char* net::http::downloader::kDefaultUrlsFile = "urls.txt";
const char* net::http::downloader::kDefaultDirectory = "data";

bool net::http::downloader::create(const char* url_file, const char* dir)
{
  size_t len;
  if ((len = strlen(url_file)) >= sizeof(_M_url_file)) {
    return false;
  }

  struct stat buf;
  if ((stat(url_file, &buf) == 0) && (!S_ISREG(buf.st_mode))) {
    return false;
  }

  const char* last_slash = NULL;
  for (const char* ptr = url_file + len - 1; ptr >= url_file; ptr--) {
    if (*ptr == '/') {
      last_slash = ptr;
      break;
    }
  }

  if (last_slash) {
    size_t dirlen = last_slash - url_file + 1;

    // Check that the directory exists.
    memcpy(_M_url_file, url_file, dirlen);

    _M_url_file[dirlen] = 0;

    if ((stat(_M_url_file, &buf) < 0) || (!S_ISDIR(buf.st_mode))) {
      return false;
    }

    memcpy(_M_url_file + dirlen, last_slash + 1, len - dirlen + 1);
  } else {
    memcpy(_M_url_file, url_file, len + 1);
  }

  // Create directory.
  if ((len = strlen(dir)) >= sizeof(_M_dir)) {
    return false;
  }

  if (mkdir(dir, 0777) < 0) {
    if (errno == EEXIST) {
      if ((stat(dir, &buf) < 0) || (!S_ISDIR(buf.st_mode))) {
        return false;
      }
    } else {
      return false;
    }
  }

  if (!_M_selector.create()) {
    return false;
  }

  if ((_M_clients = new (std::nothrow) client[_M_selector.size()]) == NULL) {
    return false;
  }

  if ((_M_requests = new (std::nothrow) request[_M_selector.size()]) == NULL) {
    return false;
  }

  if (dir[len - 1] == '/') {
    memcpy(_M_dir, dir, len - 1);
    _M_dir[len] = 0;
  } else {
    memcpy(_M_dir, dir, len + 1);
  }

  return true;
}

void net::http::downloader::start()
{
  _M_running = true;

  do {
    if (_M_selector.count() > 0) {
      if (!_M_selector.wait_for_events(1000)) {
        update_time();
      } else {
        update_time();

        _M_selector.process_events();
      }
    } else {
      sleep(1);
      update_time();
    }

    _M_scheduler.check_expired(_M_current_msec);

    // Check whether there is a new file with URLs.
    if (_M_selector.count() < _M_max_connections) {
      struct stat buf;
      if ((stat(_M_url_file, &buf) == 0) && (S_ISREG(buf.st_mode))) {
        load_urls();
      }
    }
  } while (_M_running);
}

bool net::http::downloader::load_urls()
{
  if (!_M_file) {
    if ((_M_file = fopen(_M_url_file, "r")) == NULL) {
      return false;
    }
  }

  char line[4 * 1024];
  while ((_M_selector.count() < _M_max_connections) &&
         (fgets(line, sizeof(line), _M_file))) {
    const char* begin = line;
    while ((*begin) && (*begin <= ' ')) {
      begin++;
    }

    if (!*begin) {
      continue;
    }

    const char* end = begin + 1;
    while (*end > ' ') {
      end++;
    }

    uri::uri uri;
    if (!uri.init(begin, end - begin)) {
      continue;
    }

    // If not HTTP or HTTPS...
    const string::slice& scheme(uri.scheme());
    if (((scheme.length() != 4) ||
         (strncasecmp(scheme.data(), "http", 4) != 0))
#if HAVE_SSL
        &&
        ((scheme.length() != 5) ||
         (strncasecmp(scheme.data(), "https", 5) != 0))) {
#else
       ) {
#endif
      continue;
    }

    const string::slice& host(uri.host());
    char hoststr[512];

    if (host.length() >= sizeof(hoststr)) {
      continue;
    }

    memcpy(hoststr, host.data(), host.length());
    hoststr[host.length()] = 0;

    socket_address addr;
    if (!resolve(hoststr, addr)) {
      continue;
    }

    in_port_t port = (uri.port() == 0) ?
                                         standard_port(scheme.data(),
                                                       scheme.length()) :
                                         uri.port();

    if (addr.ss_family == AF_INET) {
      reinterpret_cast<ipv4_address*>(&addr)->port(port);
    } else {
      reinterpret_cast<ipv6_address*>(&addr)->port(port);
    }

    socket sock;
    if (!sock.connect(socket::type::kStream, addr, 0)) {
      continue;
    }

    if (!_M_selector.add(sock.fd(),
                         fdtype::kFdSocket,
                         &_M_clients[sock.fd()],
                         io::event::kWrite)) {
      sock.close();
      continue;
    }

    request* req = &_M_requests[sock.fd()];
    req->clear();

    req->init(addr, method::kGet, util::move(uri));

    char path[PATH_MAX];
    struct stat buf;

    do {
      snprintf(path, sizeof(path), "%s/%012lu", _M_dir, _M_count++);
    } while (stat(path, &buf) == 0);

    client* client = &_M_clients[sock.fd()];

    client->clear();

    // Set socket descriptor.
    client->fd(sock.fd());

    if (!client->init(req, path)) {
      // The selector closes the socket.
      _M_selector.remove(sock.fd());

      continue;
    }

    // Schedule client.
    _M_scheduler.schedule(kNormalPriority,
                          client->timer(),
                          _M_current_msec + (kClientTimeout * 1000));
  }

  // If the end of the file has been reached...
  if (_M_selector.count() < _M_max_connections) {
    fclose(_M_file);
    _M_file = NULL;

    char newpath[PATH_MAX];
    struct stat buf;

    do {
      snprintf(newpath, sizeof(newpath), "%s_%06lu", _M_url_file, _M_nfiles++);
    } while (stat(newpath, &buf) == 0);

    rename(_M_url_file, newpath);
  }

  return true;
}

bool net::http::downloader::resolve(const char* host, socket_address& addr)
{
#if defined(__linux__) || defined(__sun__)
  char buf[2048];
  struct hostent ret;
#endif // defined(__linux__) || defined(__sun__)

  struct hostent* result;

#if defined(__linux__)
  int error;
  if (gethostbyname_r(host, &ret, buf, sizeof(buf), &result, &error) != 0) {
#elif defined(__FreeBSD__) || \
      defined(__OpenBSD__) || \
      defined(__NetBSD__) || \
      defined(__DragonFly__) || \
      defined(__minix)
  if ((result = gethostbyname(host)) == NULL) {
#elif defined(__sun__)
  int error;
  if ((result = gethostbyname_r(host,
                                &ret,
                                buf,
                                sizeof(buf),
                                &error)) == NULL) {
#endif
    return false;
  }

  // IPv4?
  if (result->h_addrtype == AF_INET) {
    ipv4_address* ipv4_addr = reinterpret_cast<ipv4_address*>(&addr);

    ipv4_addr->sin_family = AF_INET;
    memcpy(&ipv4_addr->sin_addr, result->h_addr_list[0], result->h_length);
    memset(ipv4_addr->sin_zero, 0, sizeof(ipv4_addr->sin_zero));
  } else {
    ipv6_address* ipv6_addr = reinterpret_cast<ipv6_address*>(&addr);

    memset(ipv6_addr, 0, sizeof(ipv6_address));

    ipv6_addr->sin6_family = AF_INET6;
    memcpy(&ipv6_addr->sin6_addr, result->h_addr_list[0], result->h_length);
  }

  return true;
}
