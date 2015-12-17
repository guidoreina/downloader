#include <unistd.h>
#include <sys/resource.h>
#include "net/fdmap.h"

net::fdmap::~fdmap()
{
  if (_M_index) {
    for (size_t i = 0; i < _M_used; i++) {
      close(_M_index[i]);
    }

    free(_M_index);
  }

  if (_M_entries) {
    free(_M_entries);
  }
}

bool net::fdmap::create()
{
  // Get the maximum number of file descriptors.
  struct rlimit rlim;
  if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
    return false;
  }

  if ((_M_entries = reinterpret_cast<struct fdentry*>(
                      malloc(rlim.rlim_cur * sizeof(struct fdentry))
                    )) == NULL) {
    return false;
  }

  if ((_M_index = reinterpret_cast<unsigned*>(
                    malloc(rlim.rlim_cur * sizeof(unsigned))
                  )) == NULL) {
    return false;
  }

  _M_size = rlim.rlim_cur;

  for (size_t i = 0; i < _M_size; i++) {
    _M_entries[i].index = -1;
  }

  return true;
}

bool net::fdmap::add(unsigned fd, fdtype type, io::event_handler* handler)
{
  if (_M_entries[fd].index != -1) {
    // Already inserted.
    return false;
  }

  _M_entries[fd].index = _M_used;
  _M_entries[fd].type = type;
  _M_entries[fd].handler = handler;

  _M_index[_M_used++] = fd;

  return true;
}

bool net::fdmap::remove(unsigned fd)
{
  int index;
  if ((index = _M_entries[fd].index) == -1) {
    // Not inserted.
    return false;
  }

  _M_entries[fd].index = -1;

  _M_used--;

  if (static_cast<size_t>(index) < _M_used) {
    _M_index[index] = _M_index[_M_used];
    _M_entries[_M_index[index]].index = index;
  }

  return true;
}
