#ifndef OPTIONS_HEADER
#define OPTIONS_HEADER

#include <support/logger.h>
#include <support/types.h>

typedef enum {
  TASK_COMPILE,
  TASK_HELP,
} CompilerTask;

typedef struct {
  CompilerTask task;
  LogLevel log_level;
  const char *input_filename;

  bool emit_ast_dot;
  const char *ast_dot_filename;

  LogLevel scanner_log_level;
  LogLevel parser_log_level;
} CompilerOptions;

CompilerOptions options(i32 argc, char *const*argv);

#endif
