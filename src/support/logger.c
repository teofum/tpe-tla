#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <support/util.h>

#include "logger.h"

#define FATAL_COLOR "\033[0;31m"
#define ERROR_COLOR "\033[1;31m"
#define WARNING_COLOR "\033[0;33m"
#define INFO_COLOR "\033[0;34m"
#define VERBOSE_COLOR "\033[0m"
#define DEBUG_COLOR "\033[0;35m"
#define RESET "\033[0m"

struct Logger {
  char *name;
  FILE *out_file;
  LogLevel level;
};

static const char *log_level_str[] = {
  [LOG_ALL] = "",
  [LOG_DEBUG] = "[" DEBUG_COLOR "DEBUG" RESET "]",
  [LOG_VERBOSE] = "[" VERBOSE_COLOR "VERBOSE" RESET "]",
  [LOG_INFO] = "[" INFO_COLOR "INFO" RESET "]",
  [LOG_WARNING] = "[" WARNING_COLOR "WARNING" RESET "]",
  [LOG_ERROR] = "[" ERROR_COLOR "ERROR" RESET "]",
  [LOG_FATAL] = "[" FATAL_COLOR "FATAL" RESET "]",
};

Logger *logger_create(const char *name, FILE *out_file, LogLevel level) {
  Logger *logger = new (Logger);
  *logger = (Logger){
    .name = malloc(1 + strlen(name)),
    .out_file = out_file,
    .level = level,
  };
  strcpy(logger->name, name);

  return logger;
}

void logger_free(Logger *logger) {
  if (!logger) return;

  if (logger->name) free(logger->name);
  free(logger);
}

void logger_logv(Logger *logger, LogLevel level, const char *format, va_list args) {
  if (level < logger->level) return;

  char *log_fmt = malloc(21 + 2 + strlen(logger->name) + strlen(format));
  sprintf(log_fmt, "[%s]%s %s\n", logger->name, log_level_str[level], format);
  vfprintf(logger->out_file, log_fmt, args);
  free(log_fmt);
}

void logger_log(Logger *logger, LogLevel level, const char *const format, ...) {
  va_list args;
  va_start(args, format);
  logger_logv(logger, level, format, args);
  va_end(args);
}
