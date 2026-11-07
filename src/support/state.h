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
  Logger *logger;

  bool **file_dependencies;
  u32 *file_order;

  CompilationStatus status;
  i32 current_file;

  Program **asts;
  SymbolTable **symbol_data;
  const Value **values;

  Scope global_scope;
} CompilerState;

CompilerState *cs_init(CompilerOptions *options);
CompilationStatus cs_shutdown();

CompilerState *cs();

void cs_build_file_order();

const char *cs_current_filepath();

#endif
