#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <stdlib.h>

#define new(T) (T *)(calloc(1, sizeof(T)))
#define new_array(T, n) (T *)(calloc(n, sizeof(T)))

#define map_or_null(x, f) ((x) ? f((x)) : NULL)
#define or_default(x, d) ((x) ? (x) : (d))

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#endif
