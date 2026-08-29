#ifndef ENVIRONMENT_HEADER
#define ENVIRONMENT_HEADER

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * Analog to "getStringOrDefault", but returning true if the string is exactly
 * "true"; false otherwise. The default value is only used when the variable
 * is undefined.
 */
const bool getBooleanOrDefault(const char * name, const bool defaultValue);

/**
 * Gets the value of an environment variable by name, or returns a default
 * value if the variable is undefined.
 * 
 * @see https://cplusplus.com/reference/cstdlib/getenv/
 */
const char * getStringOrDefault(const char * name, const char * defaultValue);

#endif
