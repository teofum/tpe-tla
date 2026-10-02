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

  u32 input_file_count;
  char **input_filenames;

  bool emit_ast_dot;
  char *ast_dot_base_path;

  LogLevel scanner_log_level;
  LogLevel parser_log_level;
} CompilerOptions;

CompilerOptions options(i32 argc, char *const*argv);
void options_free(CompilerOptions *opts);

#endif
