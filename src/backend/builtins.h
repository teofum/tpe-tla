#ifndef BUILTINS_HEADER
#define BUILTINS_HEADER

#include <ast/ast.h>
#include <support/state.h>

void builtins_init(CompilerState *cs);
void builtins_shutdown();

void builtins_register_fns(Symbols *s);
void builtins_set_eval_arena(dyn_arena *a);

#endif
