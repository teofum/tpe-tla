#ifndef SCOPE_HEADER
#define SCOPE_HEADER

#include <support/dyn_array.h>

typedef enum {
  SC_GLOBAL,
  SC_FILE,
  SC_BLOCK,
  SC_FUNCTION,
} ScopeType;

typedef struct Scope Scope;

Scope *sc_new(ScopeType type);
u32 sc_id(const Scope *scope);

void st_init();
void st_shutdown();

// Scope *st_begin();
// void st_end(Scope *st);

Scope *st_push(Scope *stack, Scope *scope);
Scope *st_pop(Scope *stack);

Scope *st_next(const Scope *stack, bool variable);

#endif
