#ifndef ENV_HEADER
#define ENV_HEADER

#include <support/logger.h>

bool env_set_str(const char *name, const char **out);
bool env_set_bool(const char *name, bool *out);
bool env_set_log_level(const char *name, LogLevel *out);

#endif
