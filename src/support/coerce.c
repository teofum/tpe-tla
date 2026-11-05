#include <stdlib.h>
#include <string.h>

#include <support/str.h>

#include "coerce.h"

i64 cstr_to_i64(const char *s, u32 base, bool *ok) {
  char *end = NULL;
  i64 r = strtoll(s, &end, base);
  if (ok) *ok = end && *end == 0;
  return r;
}

f64 cstr_to_f64(const char *s, bool *ok) {
  char *end = NULL;
  f64 r = strtod(s, &end);
  if (ok) *ok = end && *end == 0;
  return r;
}

bool cstr_to_bool(const char *s, bool *ok) {
  bool r = false;
  bool valid = false;
  if (strcasecmp(s, "true") == 0) {
    r = true;
    valid = true;
  } else if (strcasecmp(s, "false") == 0) {
    r = false;
    valid = true;
  }

  if (ok) *ok = valid;
  return r;
}

i64 str_to_i64(str s, u32 base, bool *ok) {
  char *temp = str_to_cstring(s);
  i64 r = cstr_to_i64(temp, base, ok);
  free(temp);
  return r;
}

f64 str_to_f64(str s, bool *ok) {
  char *temp = str_to_cstring(s);
  f64 r = cstr_to_f64(temp, ok);
  free(temp);
  return r;
}

bool str_to_bool(str s, bool *ok) {
  char *temp = str_to_cstring(s);
  bool r = cstr_to_bool(temp, ok);
  free(temp);
  return r;
}
