#ifndef STR_HEADER
#define STR_HEADER

#include <support/types.h>

str str_from_cstring(const char *cstring);
char *str_to_cstring(str string);

str str_slice(str string, i64 start, i64 end);
str str_clone(str string);
str str_format(const char *fmt, ...);
str str_concat(u32 n, str sep, ...);
void str_append(str *string, str second);
void str_append_temp(str *string, str second);
void str_append_c(str *string, const char *second);

void str_free(str string);

#endif
