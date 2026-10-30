#include <assert.h>
#include <stdlib.h>

#include <support/util.h>

#include "scope.h"

struct StackEntry {
  Scope scope;
  StackEntry *parent;
};
struct Stack {
  StackEntry *current;
};

static u32 _next_scope_id = 0;

Scope sc_new(ScopeType type) {
  return (Scope){ .id = _next_scope_id++, .type = type };
}

Stack *st_begin() {
  return new(Stack);
}

void st_end(Stack *st) {
  assert(st->current == NULL);
  free(st);
}

Scope st_push(Stack *st, Scope scope) {
  StackEntry *entry = new(StackEntry);
  *entry = (StackEntry){ .scope = scope, .parent = st->current };
  st->current = entry;
  return scope;
}

Scope st_pop(Stack *st) {
  assert(st->current != NULL);

  StackEntry *current = st->current;
  st->current = current->parent;
  Scope scope = current->scope;
  free(current);
  return scope;
}

Scope st_peek(Stack *st) {
  return st->current->scope;
}

StackEntry *st_current(Stack *st) {
  return st->current;
}

StackEntry *st_next(StackEntry *st) {
  // Cut off symbol lookup at function scope
  if (st->scope.type == SC_FUNCTION) return NULL;
  return st->parent;
}

Scope st_scope(StackEntry *st) {
  return st->scope;
}
