#ifndef STATE_HEADER
#define STATE_HEADER

#include <ast/ast.h>
#include <support/logger.h>
#include <support/options.h>
#include <support/types.h>

typedef struct {
  CompilerOptions options;
  CompilationStatus status;
  Program *ast;
  Logger *logger;
} CompilerState;

#endif
