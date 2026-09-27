#ifndef ENV_HEADER
#define ENV_HEADER

const char *env_str(const char *name, const char *default_value);
bool env_bool(const char *name, bool default_value);

#endif
