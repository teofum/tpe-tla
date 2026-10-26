#ifndef EVAL_HEADER
#define EVAL_HEADER

#include <ast/ast.h>
#include <support/hash_map.h>
#include <support/state.h>
#include <support/types.h>

/* -------------------------------------------------------------------------- */

void eval_init(CompilerState *cs);
void eval_shutdown();

CompilationStatus eval_evaluate(i32 file_idx);

#endif
