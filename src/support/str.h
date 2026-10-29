#ifndef STR_HEADER
#define STR_HEADER

#include <support/mem.h>
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

i32 str_compare(str a, str b);
bool str_equals(str a, str b);

void str_free(str string);

/* -------------------------------------------------------------------------- */

str str_from_cstring_a(malloc_t alloc, const char *cstring);
char *str_to_cstring_a(malloc_t alloc, str string);

str str_clone_a(malloc_t alloc, str string);
str str_format_a(malloc_t alloc, const char *fmt, ...);
str str_concat_a(malloc_t alloc, u32 n, str sep, ...);
void str_append_a(malloc_t alloc, free_t dealloc, str *string, str second);
void str_append_temp_a(malloc_t alloc, free_t dealloc, str *string, str second);
void str_append_c_a(malloc_t alloc, free_t dealloc, str *string, const char *second);

void str_free_a(free_t dealloc, str string);

#endif
