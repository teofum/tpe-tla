#ifndef STR_HEADER
#define STR_HEADER

#include <support/types.h>

str str_from_cstring(const char *cstring);
char *str_to_cstring(str string);

str str_slice(str string, i64 start, i64 end);
str str_clone(str string);

void str_free(str string);

#endif
