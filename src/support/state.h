#ifndef STATE_HEADER
#define STATE_HEADER

#include <ast/ast.h>
#include <ast/symbols.h>
#include <backend/scope.h>
#include <support/logger.h>
#include <support/options.h>
#include <support/types.h>

typedef struct {
  CompilerOptions options;
  CompilationStatus status;
  Program **asts;
  Symbols **symbol_data;
  const Value **values;
  Logger *logger;
  i32 current_file;
  Scope global_scope;
} CompilerState;

const char *state_current_filepath(CompilerState *cs);

#endif
