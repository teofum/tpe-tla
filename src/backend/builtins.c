#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <ast/ast.h>
#include <ast/symbols.h>
#include <frontend/bison/bison_parser.h>
#include <support/hash_map.h>
#include <support/mem.h>
#include <support/util.h>

#include "builtins.h"

static CompilerState *cs;
static dyn_arena *builtins_arena;
static dyn_arena *eval_arena;

/* -- Module init/shutdown -------------------------------------------------- */

void builtins_init(CompilerState *compiler_state) {
  cs = compiler_state;

  builtins_arena = mem_dyn_arena_create(MEM_SIZE_AUTO);
}

void builtins_shutdown() {
  mem_dyn_arena_destroy(builtins_arena);

  cs = NULL;
}

/* -- Memory ---------------------------------------------------------------- */

static void *eval_malloc(usize size) {
  return mem_dyn_arena_malloc(eval_arena, size);
}

static void *eval_calloc(usize count, usize size) {
  return mem_dyn_arena_calloc(eval_arena, count, size);
}

#define eval_new(T) ((T*)(eval_calloc(1, sizeof(T))))
#define eval_new_array(T, n) ((T*)(eval_calloc(n, sizeof(T))))

static void *builtins_malloc(usize size) {
  return mem_dyn_arena_malloc(builtins_arena, size);
}

static void *builtins_calloc(usize count, usize size) {
  return mem_dyn_arena_calloc(builtins_arena, count, size);
}

#define b_new(T) ((T*)(builtins_calloc(1, sizeof(T))))
#define b_new_array(T, n) ((T*)(builtins_calloc(n, sizeof(T))))

#define b_make(T, name, ...)                \
  T *name = b_new(T);                       \
  *name = (T)__VA_ARGS__

/* -- Builtins: utils ------------------------------------------------------- */

static Type *_builtin(BuiltinType builtin) {
  b_make(Type, type, { .type = T_BUILTIN, .builtin = builtin, .meta = {0} });
  return type;
}

static SymbolKey _sym_key(char *name) {
  return (SymbolKey){ .name = name, .scope = cs->global_scope.id };
}

static Identifier *static_id(char *name) {
  Identifier *id = b_new(Identifier);
  *id = (Identifier){
    .label = IDENTIFIER,
    .len = strlen(name),
    .lexeme = name,
    .loc = {0},
  };

  return id;
}

/* -- Builtins: functions --------------------------------------------------- */

static const Value *rand_fn(
  const Value **args,
  const Value *c,
  const FunctionDef *fn
) {
  Value *value = eval_new(Value);
  *value = (Value){ .type = VAL_INT, .integer = rand() };
  return value;
}

static void _register_rand_fn(FunctionTable *fns) {
  FunctionDef *def = b_new(FunctionDef);
  *def = (FunctionDef){
    .meta = {0},
    .name = static_id("rand"),
    .params_len = 0,
    .params = NULL,
    .compose_type = NULL,
    .return_type = _builtin(T_INT),
    .builtin = rand_fn,
  };

  SymbolKey key = _sym_key("rand");
  bool s = map_insert(fns, &key, def);
  assert(s);
}

/* -- Builtins: API --------------------------------------------------------- */

void builtins_register_fns(Symbols *s) {
  _register_rand_fn(s->functions);
}

void builtins_set_eval_arena(dyn_arena *a) {
  eval_arena = a;
}
