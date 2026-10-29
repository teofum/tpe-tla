#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <support/types.h>
#include <support/util.h>

#include "str.h"

static str _vstr_format_a(malloc_t alloc, const char *fmt, va_list args) {
  usize len = vsnprintf(NULL, 0, fmt, args);
  char *buf = alloc(len + 1);
  vsnprintf(buf, len + 1, fmt, args);

  return (str){ .len = len, .ptr = buf };
}

static str _vstr_concat_a(malloc_t alloc, u32 n, str sep, va_list args) {
  str *strings = new_array(str, n);
  for (u32 i = 0; i < n; i++) strings[i] = va_arg(args, str);

  usize len = 0;
  for (u32 i = 0; i < n; i++) {
    if (i > 0) len += sep.len;
    len += strings[i].len;
  }
  char *buf = alloc(len);

  char *cursor = buf;
  for (u32 i = 0; i < n; i++) {
    if (i > 0) {
      strncpy(cursor, sep.ptr, sep.len);
      cursor += sep.len;
    }
    strncpy(cursor, strings[i].ptr, strings[i].len);
    cursor += strings[i].len;
  }

  free(strings);
  return (str){ .len = len, .ptr = buf };
}

/* -------------------------------------------------------------------------- */

str str_from_cstring(const char *cstring) {
  return str_from_cstring_a(malloc, cstring);
}

char *str_to_cstring(str string) {
  return strndup(string.ptr, string.len);
}

str str_slice(str string, i64 start, i64 end) {
  if (start < 0) start += string.len;
  if (end < 0) end += string.len;
  str slice = {
    .len = end - start,
    .ptr = string.ptr + start,
  };
  return slice;
}

str str_clone(str string) {
  return str_clone_a(malloc, string);
}

str str_format(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  str string = _vstr_format_a(malloc, fmt, args);
  va_end(args);

  return string;
}

str str_concat(u32 n, str sep, ...) {
  va_list args;
  va_start(args, sep);
  str string = _vstr_concat_a(malloc, n, sep, args);
  va_end(args);

  return string;
}

void str_append(str *string, str second) {
  str_append_a(malloc, free, string, second);
}

void str_append_temp(str *string, str second) {
  str_append(string, second);
  str_free(second);
}

void str_append_c(str *string, const char *second) {
  str_append_temp(string, str_from_cstring(second));
}

i32 str_compare(str a, str b) {
  u64 len = min(a.len, b.len);
  i32 cmp = strncmp(a.ptr, b.ptr, len);
  if (cmp != 0) return cmp;
  return a.len - b.len;
}

bool str_equals(str a, str b) {
  return str_compare(a, b) == 0;
}

void str_free(str string) {
  free(string.ptr);
}

/* -------------------------------------------------------------------------- */

str str_from_cstring_a(malloc_t alloc, const char *cstring) {
  usize len = strlen(cstring);
  str string = {
    .len = len,
    .ptr = alloc(len),
  };
  strncpy(string.ptr, cstring, len); // string.ptr is NOT null terminated! This is by design.
  return string;
}

char *str_to_cstring_a(malloc_t alloc, str string) {
  char *cstring = alloc(string.len + 1);
  strncpy(cstring, string.ptr, string.len);
  cstring[string.len] = 0;
  return cstring;
}

str str_clone_a(malloc_t alloc, str string) {
  str cloned = {
    .len = string.len,
    .ptr = alloc(string.len),
  };
  strncpy(cloned.ptr, string.ptr, string.len);
  return cloned;
}

str str_format_a(malloc_t alloc, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  str string = _vstr_format_a(alloc, fmt, args);
  va_end(args);

  return string;
}

str str_concat_a(malloc_t alloc, u32 n, str sep, ...) {
  va_list args;
  va_start(args, sep);
  str string = _vstr_concat_a(alloc, n, sep, args);
  va_end(args);

  return string;
}

void str_append_a(malloc_t alloc, free_t dealloc, str *string, str second) {
  char *new_buf = alloc(string->len + second.len);
  strncpy(new_buf, string->ptr, string->len);
  strncpy(new_buf + string->len, second.ptr, second.len);
  if (dealloc) dealloc(string->ptr);
  string->len += second.len;
  string->ptr = new_buf;
}

void str_append_temp_a(malloc_t alloc, free_t dealloc, str *string, str second) {
  str_append_a(alloc, dealloc, string, second);
  str_free_a(dealloc, second);
}

void str_append_c_a(malloc_t alloc, free_t dealloc, str *string, const char *second) {
  str_append_temp_a(alloc, dealloc, string, str_from_cstring_a(alloc, second));
}

void str_free_a(free_t dealloc, str string) {
  dealloc(string.ptr);
}
