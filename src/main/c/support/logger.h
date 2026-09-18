#ifndef LOGGER_HEADER
#define LOGGER_HEADER

#include <stdio.h>

typedef enum {
  LOG_ALL,
  LOG_DEBUG,
  LOG_VERBOSE,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_FATAL,
} LogLevel;

typedef struct Logger Logger;

Logger *logger_create(const char *name, FILE *out_file);
void logger_free(Logger *logger);

void logger_logv(Logger *logger, LogLevel level, const char *const format, va_list args);
void logger_log(Logger *logger, LogLevel level, const char *const format, ...);

#endif
