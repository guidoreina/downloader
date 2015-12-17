#ifndef MACROS_H
#define MACROS_H

#define ARRAY_SIZE(x)     (sizeof(x) / sizeof(*x))

#define MAX(x, y)         (((x) > (y)) ? (x) : (y))
#define MIN(x, y)         (((x) < (y)) ? (x) : (y))
#define MIN3(x, y, z)     (((x) < (y)) ? MIN((x), (z)) : MIN((y), (z)))

#endif // MACROS_H
