#include <stdlib.h>
#include <string.h>

#include <support/logger.h>

#include "env.h"

bool env_set_str(const char *name, const char **out) {
  const char *val = getenv(name);
  if (!val) return false;

  *out = val;
  return true;
}

bool env_set_bool(const char *name, bool *out) {
  const char *val = getenv(name);
  if (!val) return false;

  *out = strcasecmp(val, "true") == 0
      || strcasecmp(val, "yes") == 0
      || strcasecmp(val, "1") == 0;

  return true;
}

bool env_set_log_level(const char *name, LogLevel *out) {
  const char *val = getenv(name);
  if (!val) return false;

  *out = logger_str_to_level(val);
  return true;
}
