/* IMPLEMENTATION NOTE ====================================================== *\
 *
 * Because this code is intended to only ever run after static analysis, it
 * makes A LOT of assumptions that certain invariants (effectively, anything
 * the static analysis pass enforces) hold.
 * It will not check for any of these invariants and may crash or fail
 * unpredictably if they do not hold for the input. This is not a bug in the
 * evaluator, but rather in the static analyzer that should have caught these.
\* ========================================================================== */

#include <assert.h>
#include <stdio.h>

#include <ast/ast.h>
#include <support/hash_map.h>
#include <support/logger.h>
#include <support/state.h>
#include <support/str.h>
#include <support/types.h>
#include <support/util.h>

#include "eval.h"

typedef struct {
  Logger *logger;
} Evaluator;

static CompilerState *cs = NULL;
static Evaluator *e = NULL;

/* -- Module init/shutdown -------------------------------------------------- */

void eval_init(CompilerState *compiler_state) {
  cs = compiler_state;

  e = new(Evaluator);
  e->logger = logger_create("Eval", stderr, LOG_DEBUG);
}

void eval_shutdown() {
  if (!e) return;

  if (e->logger) logger_free(e->logger);
  free(e);

  e = NULL;
  cs = NULL;
}

/* -- Evaluation: utils ----------------------------------------------------- */

#define eval_new(T) new(T)
#define eval_new_array(T, n) new_array(T, n)

#define eval_make(T, name, ...)               \
  T *name = eval_new(T);                      \
  *name = (T)__VA_ARGS__

static usize eval_hash_value(const Value *value) {
  return 4; // xkcd 221 certified hash, emudhra approved
}

static bool eval_compare_values(const Value *a, const Value *b);

static bool eval_compare_lists(const List *a, const List *b) {
  if (a->len != b->len) return false;
  for (u32 i = 0; i < a->len; i++) {
    if (!eval_compare_values(a->items[i], b->items[i])) return false;
  }
  return true;
}

static bool eval_compare_maps(const Map *a, const Map *b) {
  return false; // TODO lol
}

static bool eval_compare_elements(const Element *a, const Element *b) {
  return false; // TODO lol
}

static bool eval_compare_values(const Value *a, const Value *b) {
  if (a->type != b->type) return false;

  switch (a->type) {
    case VAL_NIL: return true;
    case VAL_INT: return a->integer == b->integer;
    case VAL_FLOAT: return a->floating == b->floating;
    case VAL_BOOL: return a->boolean == b->boolean;
    case VAL_STRING: return str_equals(*a->string, *b->string);
    case VAL_LIST: return eval_compare_lists(a->list, b->list);
    case VAL_MAP: return eval_compare_maps(a->map, b->map);
    case VAL_ELEMENT: return eval_compare_elements(a->element, b->element);
    case VAL_RANGE: {
      return a->range->start == b->range->start
        && a->range->end == b->range->end;
    }
  }
}

static Value _nil_value = { .type = VAL_NIL };
static const Value *_nil() {
  return &_nil_value;
}

static const Value *_int(i64 int_value) {
  eval_make(Value, value, { .type = VAL_INT, .integer = int_value });
  return value;
}

static const Value *_float(f64 float_value) {
  eval_make(Value, value, { .type = VAL_FLOAT, .floating = float_value });
  return value;
}

static const Value *_bool(bool bool_value) {
  eval_make(Value, value, { .type = VAL_BOOL, .boolean = bool_value });
  return value;
}

static const Value *_str(const str *str_value) {
  eval_make(Value, value, { .type = VAL_BOOL, .string = str_value });
  return value;
}

static const Value *_str_id(const Identifier *id) {
  str *str_value = eval_new(str);
  *str_value = (str){ .ptr = id->lexeme, .len = id->len };
  return _str(str_value);
}

/* -- Evaluation ------------------------------------------------------------ */

static const Value *eval_expr(const Expr *expr);

static const Value *eval_range(const RangeLiteral *range) {
  const Value *start_v = eval_expr(range->start);
  const Value *end_v = eval_expr(range->end);
  assert(start_v->type == VAL_INT);
  assert(end_v->type == VAL_INT);

  eval_make(Range, range_v, {
    .start = start_v->integer,
    .end = end_v->integer,
  });
  eval_make(Value, value, { .type = VAL_RANGE, .range = range_v });
  return value;
}

static const Value *eval_list(const ListLiteral *list) {
  eval_make(List, list_v, {
    .len = list->len,
    .items = eval_new_array(const Value *, list->len)
  });

  for (u32 i = 0; i < list->len; i++) {
    list_v->items[i] = eval_expr(list->exprs[i]);
  }

  eval_make(Value, value, { .type = VAL_LIST, .list = list_v });
  return value;
}

static const Value *eval_vector(const VectorLiteral *vec) {
  eval_make(List, list_v, {
    .len = vec->len,
    .items = eval_new_array(const Value *, vec->len)
  });

  const Value *item_val = eval_expr(vec->value);
  for (u32 i = 0; i < vec->len; i++) list_v->items[i] = item_val;

  eval_make(Value, value, { .type = VAL_LIST, .list = list_v });
  return value;
}

static const Value *eval_map(const MapLiteral *map) {
  Map *map_v = eval_new(Map);
  map_init(map_v, eval_hash_value, eval_compare_values);

  for (u32 i = 1; i < map->len; i++) {
    const Value *key_v = eval_expr(map->entries[i]->key);
    const Value *value_v = eval_expr(map->entries[i]->value);
    if (!map_insert(map_v, (Value *)key_v, (Value *)value_v)) {
      // TODO duplicate key in map literal error
      return _nil();
    }
  }

  eval_make(Value, value, { .type = VAL_MAP, .map = map_v });
  return value;
}

static const Value *eval_struct(const StructLiteral *struct_l) {
  Map *map_v = eval_new(Map);
  map_init(map_v, eval_hash_value, eval_compare_values);

  for (u32 i = 1; i < struct_l->len; i++) {
    const Value *key_v = _str_id(struct_l->fields[i]->name);
    const Value *value_v = eval_expr(struct_l->fields[i]->value);
    if (!map_insert(map_v, (Value *)key_v, (Value *)value_v)) {
      // TODO duplicate key in map literal error
      return _nil();
    }
  }

  eval_make(Value, value, { .type = VAL_MAP, .map = map_v });
  return value;
}

static const Value *eval_literal(const LiteralExpr *expr) {
  switch (expr->type) {
    case L_NIL: return _nil();
    case L_INTEGER: return _int(expr->integer->value);
    case L_FLOAT: return _float(expr->floating->value);
    case L_BOOL: return _bool(expr->boolean->value);
    case L_STRING: return _str(&expr->string->value);
    case L_RANGE: return eval_range(expr->range);
    case L_LIST: return eval_list(expr->list);
    case L_VECTOR: return eval_vector(expr->vec);
    case L_MAP: return eval_map(expr->map);
    case L_STRUCT: return eval_struct(expr->struct_literal);
    case L_ENUM: assert(false); // TODO
  }
}

static const Value *eval_expr(const Expr *expr) {
  switch (expr->type) {
    case EXPR_LITERAL: return eval_literal(expr->literal);
  }
  assert(false); // perish
}

CompilationStatus eval_evaluate(i32 file_idx) {
  return STATUS_SUCCEEDED;
}
