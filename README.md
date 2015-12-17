downloader
==========

downloader is an asynchronous event-driven downloader for HTTP/HTTPS which runs under Linux, FreeBSD and Solaris.

downloader checks periodically whether there is a new file with URLs, when one is found, it downloads `<max-connections>` URLs at a time and saves them in the directory `<data>`.

The format of the saved files is:

```
 <URL>
 Status-Line
 *(message-header)
 <empty-line>
 [message-body]
```


The usage is:

```
Usage: ./downloader [OPTIONS]

Options:
  --urls-file <filename> (default: urls.txt).
  --dir <directory> (default: data/).
  --max-connections <max-connections> (1 - 2048, default: 100).
  --user-agent <user-agent> (default: "").
```
