#ifndef MACROS_H
#define MACROS_H

#define ARRAY_SIZE(x)     (sizeof(x) / sizeof(*x))

#ifndef MAX
  #define MAX(x, y)       (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
  #define MIN(x, y)       (((x) < (y)) ? (x) : (y))
#endif

#define MIN3(x, y, z)     (((x) < (y)) ? MIN((x), (z)) : MIN((y), (z)))

#endif // MACROS_H
