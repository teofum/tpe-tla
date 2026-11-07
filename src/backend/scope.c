#include <assert.h>
#include <stdlib.h>

#include <support/mem.h>
#include <support/state.h>
#include <support/util.h>

#include "scope.h"

struct Scope {
  u32 id;
  ScopeType type;
  Scope *parent;
};

static u32 _next_scope_id = 0;
static dyn_arena *scope_arena = NULL;

void st_init() {
  scope_arena = mem_dyn_arena_create(MEM_SIZE_AUTO);
  cs()->global_scope = sc_new(SC_GLOBAL);
}

void st_shutdown() {
  mem_dyn_arena_destroy(scope_arena);
  scope_arena = NULL;
}

void *st_calloc(usize count, usize size) {
  return mem_dyn_arena_calloc(scope_arena, count, size);
}

#define st_new(T) ((T*)(st_calloc(1, sizeof(T))))
#define st_new_array(T, n) ((T*)(st_calloc(n, sizeof(T))))

Scope *sc_new(ScopeType type) {
  Scope *scope = st_new(Scope);
  *scope = (Scope){ .id = _next_scope_id++, .type = type };
  return scope;
}

u32 sc_id(const Scope *scope) {
  return scope->id;
}

// Scope *st_begin() {
//   return new(Stack);
// }
//
// void st_end(Stack *st) {
//   assert(st->current == NULL);
//   free(st);
// }

Scope *st_push(Scope *stack, Scope *scope) {
  scope->parent = stack;
  return scope;
}

Scope *st_pop(Scope *stack) {
  return stack->parent;
}

Scope *st_next(const Scope *stack, bool variable) {
  // Cut off symbol lookup at function scope
  if (variable && stack->type == SC_FUNCTION) return NULL;
  return stack->parent;
}
