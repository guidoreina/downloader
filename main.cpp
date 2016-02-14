#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "net/http/downloader.h"
#include "util/number.h"

#if HAVE_SSL
  #include "net/ssl_socket.h"
#endif

static net::http::downloader downloader;

static void usage(const char* program);
static void signal_handler(int nsignal);

int main(int argc, const char** argv)
{
  const char* urls_file = net::http::downloader::kDefaultUrlsFile;
  const char* dir = net::http::downloader::kDefaultDirectory;

#if defined(__OpenBSD__)
  #if defined(__x86_64__)
    uint64_t max_connections = net::http::downloader::kDefaultConnections;
  #else
    uint32_t max_connections = net::http::downloader::kDefaultConnections;
  #endif
#else
  size_t max_connections = net::http::downloader::kDefaultConnections;
#endif

  const char* user_agent = NULL;

  // Check arguments.
  int i = 1;
  while (i < argc) {
    if (strcasecmp(argv[i], "--urls-file") == 0) {
      // Last argument?
      if (i + 1 == argc) {
        usage(argv[0]);
        return -1;
      }

      urls_file = argv[i + 1];

      i += 2;
    } else if (strcasecmp(argv[i], "--dir") == 0) {
      // Last argument?
      if (i + 1 == argc) {
        usage(argv[0]);
        return -1;
      }

      dir = argv[i + 1];

      i += 2;
    } else if (strcasecmp(argv[i], "--max-connections") == 0) {
      // Last argument?
      if (i + 1 == argc) {
        usage(argv[0]);
        return -1;
      }

      if (util::number::parse(argv[i + 1],
                              strlen(argv[i + 1]),
                              max_connections,
                              net::http::downloader::kMinConnections,
                              net::http::downloader::kMaxConnections) !=
          util::number::parse_result::kSucceeded) {
        usage(argv[0]);
        return -1;
      }

      i += 2;
    } else if (strcasecmp(argv[i], "--user-agent") == 0) {
      // Last argument?
      if (i + 1 == argc) {
        usage(argv[0]);
        return -1;
      }

      user_agent = argv[i + 1];

      i += 2;
    } else {
      usage(argv[0]);
      return -1;
    }
  }

  if (!downloader.create(urls_file, dir)) {
    fprintf(stderr, "Couldn't create downloader.\n");
    return -1;
  }

  if ((user_agent) && (!downloader.user_agent(user_agent))) {
    fprintf(stderr, "Couldn't set User-Agent.\n");
    return -1;
  }

  downloader.max_connections(max_connections);

#if HAVE_SSL
  if (!net::ssl_socket::init_ssl_library()) {
    fprintf(stderr, "Couldn't initialize SSL library.\n");
    return -1;
  }
#endif // HAVE_SSL

  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &act, NULL);

  act.sa_handler = signal_handler;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);

  downloader.start();

#if HAVE_SSL
  net::ssl_socket::free_ssl_library();
#endif

  return 0;
}

void usage(const char* program)
{
  printf("Usage: %s [OPTIONS]\n", program);
  printf("\n");
  printf("Options:\n");
  printf("\t--urls-file <filename> (default: %s).\n",
         net::http::downloader::kDefaultUrlsFile);

  printf("\t--dir <directory> (default: %s/).\n",
         net::http::downloader::kDefaultDirectory);

  printf("\t--max-connections <max-connections> (%lu - %lu, default: %lu).\n",
         net::http::downloader::kMinConnections,
         net::http::downloader::kMaxConnections,
         net::http::downloader::kDefaultConnections);

  printf("\t--user-agent <user-agent> (default: \"\").\n");
  printf("\n");
}

void signal_handler(int nsignal)
{
  fprintf(stderr, "Signal received...\n");

  downloader.stop();
}
