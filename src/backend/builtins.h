#ifndef BUILTINS_HEADER
#define BUILTINS_HEADER

#include <ast/ast.h>
#include <support/state.h>

void builtins_init();
void builtins_shutdown();

void builtins_register_fns(SymbolTable *s);
void builtins_set_eval_arena(dyn_arena *a);

#endif
