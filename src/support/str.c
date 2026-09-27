#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <support/types.h>
#include <support/util.h>

#include "str.h"

str str_from_cstring(const char *cstring) {
  usize len = strlen(cstring);
  str string = {
    .len = len,
    .ptr = new_array(char, len),
  };
  strncpy(string.ptr, cstring, len); // string.ptr is NOT null terminated! This is by design.
  return string;
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
  str cloned = {
    .len = string.len,
    .ptr = new_array(char, string.len),
  };
  strncpy(cloned.ptr, string.ptr, string.len);
  return cloned;
}

str str_format(const char *fmt, ...) {
  va_list args;
  char *buf = NULL;
  va_start(args, fmt);
  usize len = vasprintf(&buf, fmt, args);
  va_end(args);

  return (str){ .len = len, .ptr = buf };
}

str str_concat(u32 n, str sep, ...) {
  va_list args;
  usize len = 0;

  va_start(args, sep);
  for (u32 i = 0; i < n; i++) {
    str next = va_arg(args, str);
    len += next.len;
  }
  char *buf = malloc(len);
  va_end(args);

  va_start(args, sep);
  char *cursor = buf;
  for (u32 i = 0; i < n; i++) {
    str next = va_arg(args, str);
    strncpy(cursor, next.ptr, next.len);
    cursor += next.len;
  }
  va_end(args);

  return (str){ .len = len, .ptr = buf };
}

void str_append(str *string, str second) {
  char *new_buf = malloc(string->len + second.len);
  strncpy(new_buf, string->ptr, string->len);
  strncpy(new_buf + string->len, second.ptr, second.len);
  free(string->ptr);
  string->len += second.len;
  string->ptr = new_buf;
}

void str_append_temp(str *string, str second) {
  str_append(string, second);
  str_free(second);
}

void str_append_c(str *string, const char *second) {
  str_append_temp(string, str_from_cstring(second));
}

void str_free(str string) {
  free(string.ptr);
}
