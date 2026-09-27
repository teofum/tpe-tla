#include <stdlib.h>
#include <string.h>

#include "env.h"

const char *env_str(const char *name, const char *default_value) {
  const char *val = getenv(name);
  if (!val) return default_value;
  return val;
}

bool env_bool(const char *name, bool default_value) {
  const char *val = getenv(name);
  if (!val) return default_value;

  return strcasecmp(val, "true") == 0
      || strcasecmp(val, "yes") == 0
      || strcasecmp(val, "1") == 0;
}
