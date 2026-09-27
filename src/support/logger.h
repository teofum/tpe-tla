#ifndef LOGGER_HEADER
#define LOGGER_HEADER

#include <stdio.h>

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define K "\033[30m"
#define R "\033[31m"
#define G "\033[32m"
#define Y "\033[33m"
#define B "\033[34m"
#define M "\033[35m"
#define C "\033[36m"
#define W "\033[37m"
#define BK "\033[90m"
#define BR "\033[91m"
#define BG "\033[92m"
#define BY "\033[93m"
#define BB "\033[94m"
#define BM "\033[95m"
#define BC "\033[96m"
#define BW "\033[97m"

typedef enum {
  LOG_ALL,
  LOG_DEBUG,
  LOG_VERBOSE,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_FATAL,
} LogLevel;

typedef enum {
  LOGGER_NONE           = 0,
  LOGGER_LOG_NAME       = 1,
  LOGGER_LOG_LEVEL      = 1 << 1,
} LoggerFlags;

typedef struct Logger Logger;

Logger *logger_create(const char *name, FILE *out_file, LogLevel level);
void logger_free(Logger *logger);

void logger_set_flags(Logger *logger, LoggerFlags flags, bool active);

void logger_logv(Logger *logger, LogLevel level, const char *const format, va_list args);
void logger_log(Logger *logger, LogLevel level, const char *const format, ...);

#endif
