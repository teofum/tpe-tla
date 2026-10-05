#ifndef STATE_HEADER
#define STATE_HEADER

#include <ast/ast.h>
#include <ast/symbols.h>
#include <support/logger.h>
#include <support/options.h>
#include <support/types.h>

typedef struct {
  CompilerOptions options;
  CompilationStatus status;
  Program **asts;
  Symbols **symbol_data;
  Logger *logger;
  i32 current_file;
} CompilerState;

const char *state_current_filepath(CompilerState *cs);

#endif
