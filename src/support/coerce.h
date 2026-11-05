#ifndef COERCE_HEADER
#define COERCE_HEADER

#include <support/types.h>

i64 cstr_to_i64(const char *s, u32 base, bool *ok);
f64 cstr_to_f64(const char *s, bool *ok);
bool cstr_to_bool(const char *s, bool *ok);

i64 str_to_i64(str s, u32 base, bool *ok);
f64 str_to_f64(str s, bool *ok);
bool str_to_bool(str s, bool *ok);

#endif
