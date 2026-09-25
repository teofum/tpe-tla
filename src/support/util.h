#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <stdlib.h>

#define new(T) calloc(1, sizeof(T))
#define new_array(T, n) calloc(n, sizeof(T))

#define map(x, f) ((x) ? f((x)) : NULL)

#endif
