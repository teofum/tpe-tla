#include <assert.h>
#include <stdarg.h>
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

static Type *_list_of(Type *item_type) {
  b_make(ListType, list, { .item_type = item_type, .meta = {0} });
  b_make(Type, type, { .type = T_LIST, .list = list, .meta = {0} });
  return type;
}

static Type *_tuple_of(Type **types, u32 len) {
  b_make(TupleType, tuple, {
    .types = types,
    .len = len,
    .meta = {0},
  });
  b_make(Type, type, { .type = T_TUPLE, .tuple = tuple, .meta = {0} });
  return type;
}

static Type *_vector_of(Type *item_type, u32 count) {
  b_make(VectorType, vec, {
    .item_type = item_type,
    .len = count,
    .meta = {0},
  });
  b_make(Type, type, { .type = T_VECTOR, .vec = vec, .meta = {0} });
  return type;
}

static Type *_map_of(Type *key_type, Type *value_type) {
  b_make(MapType, map, {
    .key_type = key_type,
    .value_type = value_type,
    .meta = {0}
  });
  b_make(Type, type, { .type = T_MAP, .map = map, .meta = {0} });
  return type;
}

static Type *_union_of(u32 n, ...) {
  b_make(UnionType, union_type, {
    .len = n,
    .types = b_new_array(Type *, n),
    .meta = {0},
  });

  va_list args;
  va_start(args, n);
  for (u32 i = 0; i < n; i++) union_type->types[i] = va_arg(args, Type *);
  va_end(args);

  b_make(Type, type, {
    .type = T_UNION,
    .union_type = union_type,
    .meta = {0}
  });
  return type;
}


static SymbolKey _sym_key(const char *name) {
  return (SymbolKey){ .name = (char *)name, .scope = cs->global_scope.id };
}

static Identifier *static_id(const char *name) {
  Identifier *id = b_new(Identifier);
  *id = (Identifier){
    .label = IDENTIFIER,
    .len = strlen(name),
    .lexeme = (char *)name,
    .loc = {0},
  };

  return id;
}

static const Value *_to_list(const Value *val) {
  if (val->type == VAL_LIST) return val;

  Value *list_v = eval_new(Value);
  *list_v = (Value){ .type = VAL_LIST, .list = eval_new(List) };
  *list_v->list = (List){
    .len = 1,
    .items = eval_new_array(const Value *, 1),
  };
  list_v->list->items[0] = val;

  return list_v;
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

static const Value *element_fn(
  const Value **args,
  const Value *c,
  const FunctionDef *fn
) {
  Element *element = eval_new(Element);
  *element = (Element){
    .tag = fn->name->lexeme,
    .self_closing = false,
    .attributes = NULL,
    .children = _to_list(c)->list,
  };

  Value *value = eval_new(Value);
  *value = (Value){ .type = VAL_ELEMENT, .element = element };
  return value;
}

static void _register_element_fn(
  FunctionTable *fns,
  const char *tag,
  bool self_closing
) {
  Type *children_type = _union_of(7,
    _builtin(T_INT),
    _builtin(T_FLOAT),
    _builtin(T_BOOL),
    _builtin(T_STRING),
    _builtin(T_NIL),
    _builtin(T_ELEMENT),
    _list_of(_union_of(6,
      _builtin(T_INT),
      _builtin(T_FLOAT),
      _builtin(T_BOOL),
      _builtin(T_STRING),
      _builtin(T_NIL),
      _builtin(T_ELEMENT)
    ))
  );

  FunctionDef *def = b_new(FunctionDef);
  *def = (FunctionDef){
    .meta = {0},
    .name = static_id(tag),
    .params_len = 0,
    .params = NULL,
    .compose_type = self_closing ? NULL : children_type,
    .return_type = _builtin(T_ELEMENT),
    .builtin = element_fn,
  };

  SymbolKey key = _sym_key(tag);
  bool s = map_insert(fns, &key, def);
  assert(s);
}

/* -- Builtins: API --------------------------------------------------------- */

void builtins_register_fns(Symbols *s) {
  _register_rand_fn(s->functions);

  _register_element_fn(s->functions, "html", false);
  _register_element_fn(s->functions, "head", false);
  _register_element_fn(s->functions, "body", false);
  _register_element_fn(s->functions, "div", false);
}

void builtins_set_eval_arena(dyn_arena *a) {
  eval_arena = a;
}
