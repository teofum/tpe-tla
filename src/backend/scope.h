#ifndef SCOPE_HEADER
#define SCOPE_HEADER

#include <support/dyn_array.h>

typedef enum {
  SC_FILE,
  SC_BLOCK,
  SC_FUNCTION,
} ScopeType;

typedef struct {
  u32 id;
  ScopeType type;
} Scope;

typedef struct StackEntry StackEntry;
typedef struct Stack Stack;

Scope sc_new(ScopeType type);

Stack *st_begin();
void st_end(Stack *st);

Scope st_push(Stack *st, Scope scope);
Scope st_pop(Stack *st);
Scope st_peek(Stack *st);

StackEntry *st_current(Stack *st);
StackEntry *st_next(StackEntry *st);
Scope st_scope(StackEntry *st);

#endif
