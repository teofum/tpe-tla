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

void str_free(str string) {
  free(string.ptr);
}
