#include <stdlib.h>
#include <stdio.h>
#include "net/http/downloaded_file_processor.h"

int main(int argc, const char** argv)
{
  // Check usage.
  if (argc != 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return -1;
  }

  net::http::downloaded_file_processor downloaded_file_processor;

  // Open downloaded file.
  if (!downloaded_file_processor.open(argv[1])) {
    fprintf(stderr, "Error opening file '%s'.\n", argv[1]);
    return -1;
  }

  // Read title.
  string::slice title;
  if (downloaded_file_processor.read_title(title)) {
    printf("Title: '%.*s'.\n\n", title.length(), title.data());
  }

  if (downloaded_file_processor.get_content_type() !=
      net::http::downloaded_file_processor::content_type::kOther) {
    // Configure downloaded file processor.
    net::http::downloaded_file_processor::configuration config;
    config.url_separators = " \"'\t\r\n,";

    downloaded_file_processor.configure(config);

    // Extract URLs.
    net::uri::uri uri;
    while (downloaded_file_processor.next(uri)) {
      net::uri::uri normalized_uri;
      if (uri.normalize(normalized_uri)) {
        string::slice str = normalized_uri.string();
        printf("%.*s\n", str.length(), str.data());
      }
    }
  }

  return 0;
}
