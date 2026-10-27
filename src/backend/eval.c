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
#include <support/dyn_array.h>
#include <support/hash_map.h>
#include <support/logger.h>
#include <support/mem.h>
#include <support/state.h>
#include <support/str.h>
#include <support/types.h>
#include <support/util.h>

#include "eval.h"
#include "frontend/bison/bison_parser.h"

typedef struct {
  Logger *logger;

  dyn_array *stack;
  dyn_arena *arena;
} Evaluator;

static CompilerState *cs = NULL;
static Evaluator *e = NULL;

/* -- Scope ----------------------------------------------------------------- */

static u32 _push_scope(u32 scope) {
  dyn_array_push(e->stack, &scope);
  return scope;
}

static void _pop_scope() {
  dyn_array_remove_unordered(e->stack, dyn_array_size(e->stack) - 1);
}

static u32 _peek_scope() {
  return *(u32 *)dyn_array_get(e->stack, dyn_array_size(e->stack) - 1);
}

static u32 _get_scope(u32 level) {
  return *(u32 *)dyn_array_get(e->stack, dyn_array_size(e->stack) - 1 - level);
}

/* -- Module init/shutdown -------------------------------------------------- */

void eval_init(CompilerState *compiler_state) {
  cs = compiler_state;

  e = new(Evaluator);
  e->logger = logger_create("Eval", stderr, LOG_DEBUG);
  e->stack = new_dyn_array(u32);
  e->arena = mem_dyn_arena_create(MEM_SIZE_AUTO);
}

void eval_shutdown() {
  if (!e) return;

  if (e->logger) logger_free(e->logger);
  if (e->stack) dyn_array_free(e->stack);
  if (e->arena) mem_dyn_arena_destroy(e->arena);
  free(e);

  e = NULL;
  cs = NULL;
}

/* -- Evaluation: utils ----------------------------------------------------- */

static void *eval_calloc(usize count, usize size) {
  return mem_dyn_arena_calloc(e->arena, count, size);
}

#define eval_new(T) ((T*)(eval_calloc(1, sizeof(T))))
#define eval_new_array(T, n) ((T*)(eval_calloc(n, sizeof(T))))

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
  eval_make(Value, value, { .type = VAL_STRING, .string = str_value });
  return value;
}

static const Value *_str_id(const Identifier *id) {
  str *str_value = eval_new(str);
  *str_value = (str){ .ptr = id->lexeme, .len = id->len };
  return _str(str_value);
}

static const Value *_list(usize len) {
  eval_make(List, list_v, {
    .len = len,
    .items = eval_new_array(const Value *, len)
  });

  eval_make(Value, value, { .type = VAL_LIST, .list = list_v });
  return value;
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
  const Value *value = _list(list->len);
  for (u32 i = 0; i < list->len; i++) {
    value->list->items[i] = eval_expr(list->exprs[i]);
  }

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
  map_inita(map_v, eval_hash_value, eval_compare_values, eval_calloc, NULL);

  for (u32 i = 0; i < map->len; i++) {
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
  map_inita(map_v, eval_hash_value, eval_compare_values, eval_calloc, NULL);

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

static const Value *eval_unary(const UnaryExpr *expr) {
  const Value *inner = eval_expr(expr->expr);
  if (!inner) return NULL;

  switch (expr->op) {
    case NOT:
    case BANG: {
      assert(inner->type == VAL_BOOL);
      return _bool(!inner->boolean);
    }
    case MINUS: {
      switch (inner->type) {
        case VAL_INT: return _int(-inner->integer);
        case VAL_FLOAT: return _float(-inner->floating);
        default: assert(false);
      }
    }
    default: assert(false); // Unreachable
  }
}

static const i64 _math_op_int(i64 l, i64 r, TokenLabel op) {
  switch (op) {
    case PLUS: return l + r;
    case MINUS: return l - r;
    case STAR: return l * r;
    case SLASH: return l / r;
    default: assert(false);
  }
}

static const f64 _math_op_float(f64 l, f64 r, TokenLabel op) {
  switch (op) {
    case PLUS: return l + r;
    case MINUS: return l - r;
    case STAR: return l * r;
    case SLASH: return l / r;
    default: assert(false);
  }
}

static const Value *eval_math_float(f64 l, const Value *r, TokenLabel op) {
  switch (r->type) {
    case VAL_INT: return _float(_math_op_float(l, (f64)r->integer, op));
    case VAL_FLOAT: return _float(_math_op_float(l, r->floating, op));
    case VAL_LIST: {
      const Value *value = _list(r->list->len);
      for (u32 i = 0; i < r->list->len; i++) {
        value->list->items[i] = eval_math_float(l, r->list->items[i], op);
      }
      return value;
    }
    default: assert(false);
  }
}

static const Value *eval_math_int(i64 l, const Value *r, TokenLabel op) {
  switch (r->type) {
    case VAL_INT: return _int(_math_op_int(l, r->integer, op));
    case VAL_FLOAT: return _float(_math_op_float((f64)l, r->floating, op));
    case VAL_LIST: {
      const Value *value = _list(r->list->len);
      for (u32 i = 0; i < r->list->len; i++) {
        value->list->items[i] = eval_math_int(l, r->list->items[i], op);
      }
      return value;
    }
    default: assert(false);
  }
}

static const Value *_math_vec_item(const Value *l, const Value *r, TokenLabel op) {
  switch (l->type) {
    case VAL_INT: return eval_math_int(l->integer, r, op);
    case VAL_FLOAT: return eval_math_float(l->floating, r, op);
    default: assert(false);
  }
}

static const Value *eval_math_vec(const List *l, const Value *r, TokenLabel op) {
  const Value *value = _list(l->len);
  for (u32 i = 0; i < l->len; i++) {
    const Value *l_item = l->items[i];
    const Value *r_item = (r->type == VAL_LIST) ? r->list->items[i] : r;
    value->list->items[i] = _math_vec_item(l_item, r_item, op);
  }
  return value;
}

static const Value *eval_string_op(const str *l, const Value *r, TokenLabel op) {
  switch (op) {
    case PLUS: // TODO
    case STAR: // TODO
    default: assert(false);
  }
}

static bool _cmp_int(i64 l, i64 r, TokenLabel op) {
  switch (op) {
    case GREATER_EQUAL: return l >= r;
    case GREATER: return l > r;
    case LESS_EQUAL: return l <= r;
    case LESS: return l < r;
    default: assert(false);
  }
}

static bool _cmp_float(f64 l, f64 r, TokenLabel op) {
  switch (op) {
    case GREATER_EQUAL: return l >= r;
    case GREATER: return l > r;
    case LESS_EQUAL: return l <= r;
    case LESS: return l < r;
    default: assert(false);
  }
}

static const Value *eval_comp_float(f64 l, const Value *r, TokenLabel op) {
  switch (r->type) {
    case VAL_INT: return _bool(_cmp_float(l, (f64)r->integer, op));
    case VAL_FLOAT: return _bool(_cmp_float(l, r->floating, op));
    default: assert(false);
  }
}

static const Value *eval_comp_int(i64 l, const Value *r, TokenLabel op) {
  switch (r->type) {
    case VAL_INT: return _bool(_cmp_int(l, r->integer, op));
    case VAL_FLOAT: return _bool(_cmp_float((f64)l, r->floating, op));
    default: assert(false);
  }
}

static const Value *eval_comp_str(const str *l, const Value *r, TokenLabel op) {
  assert(r->type == VAL_STRING);
  i32 cmp = str_compare(*l, *r->string);
  switch (op) {
    case GREATER_EQUAL: return _bool(cmp >= 0);
    case GREATER: return _bool(cmp > 0);
    case LESS_EQUAL: return _bool(cmp <= 0);
    case LESS: return _bool(cmp < 0);
    default: assert(false);
  }
}

static const Value *eval_binary(const BinaryExpr *expr) {
  const Value *left = eval_expr(expr->left);
  if (!left) return NULL;

  switch (expr->op) {
    case PLUS:
    case STAR: {
      const Value *right = eval_expr(expr->right);
      if (!right) return NULL;

      switch (left->type) {
        case VAL_INT: return eval_math_int(left->integer, right, expr->op);
        case VAL_FLOAT: return eval_math_float(left->floating, right, expr->op);
        case VAL_LIST: return eval_math_vec(left->list, right, expr->op);
        case VAL_STRING: return eval_string_op(left->string, right, expr->op);
        default: assert(false);
      }
    }
    case MINUS:
    case SLASH: {
      const Value *right = eval_expr(expr->right);
      if (!right) return NULL;

      switch (left->type) {
        case VAL_INT: return eval_math_int(left->integer, right, expr->op);
        case VAL_FLOAT: return eval_math_float(left->floating, right, expr->op);
        case VAL_LIST: return eval_math_vec(left->list, right, expr->op);
        default: assert(false);
      }
    }
    case GREATER_EQUAL:
    case GREATER:
    case LESS_EQUAL:
    case LESS: {
      const Value *right = eval_expr(expr->right);
      if (!right) return NULL;

      switch (left->type) {
        case VAL_INT: return eval_comp_int(left->integer, right, expr->op);
        case VAL_FLOAT: return eval_comp_float(left->floating, right, expr->op);
        case VAL_STRING: return eval_comp_str(left->string, right, expr->op);
        default: assert(false);
      }
    }
    case EQUAL_EQUAL:
    case BANG_EQUAL: {
      const Value *right = eval_expr(expr->right);
      if (!right) return NULL;

      return _bool(eval_compare_values(left, right));
    }
    case AND: {
      assert(left->type == VAL_BOOL);
      if (!left) return _bool(false);

      const Value *right = eval_expr(expr->right);
      if (!right) return NULL;

      assert(right->type == VAL_BOOL);
      return right;
    }
    case OR: {
      assert(left->type == VAL_BOOL);
      if (left) return _bool(true);

      const Value *right = eval_expr(expr->right);
      if (!right) return NULL;

      assert(right->type == VAL_BOOL);
      return right;
    }
    default: assert(false); // Unreachable
  }
}

static const Value *eval_expr(const Expr *expr) {
  switch (expr->type) {
    case EXPR_LITERAL: return eval_literal(expr->literal);
    case EXPR_UNARY: return eval_unary(expr->unary);
    case EXPR_BINARY: return eval_binary(expr->binary);
  }
  assert(false); // perish
}

static const Value *eval_stmt(const Stmt *stmt) {
  switch (stmt->type) {
    case STMT_EXPR: return eval_expr(stmt->expr);
  }
  assert(false); // perish
}

static const Value *eval_program(const Program *program) {
  _push_scope(program->scope);

  const Value *value = NULL;
  if (program->len == 1) {
    value = eval_stmt(program->statements[0]);
  } else {
    eval_make(List, vlist, {
      .len = program->len,
      .items = eval_new_array(const Value *, program->len)
    });
    for (u32 i = 0; i < program->len; i++) {
      vlist->items[i] = eval_stmt(program->statements[i]);
    }
    eval_make(Value, v, { .type = VAL_LIST, .list = vlist });
    value = v;
  }

  _pop_scope();
  return value;
}

CompilationStatus eval_evaluate(i32 file_idx) {
  cs->current_file = file_idx;

  Program *ast = cs->asts[file_idx];
  assert(ast != NULL); // If we got to this point without an AST we have a bug

  Symbols *sym = cs->symbol_data[file_idx];
  assert(sym != NULL); // If we got to this point without a symbol table we have a bug

  dyn_array_clear(e->stack);
  // mem_dyn_arena_free_all(sa->tc_arena);
  const Value *val = eval_program(ast);
  assert(dyn_array_size(e->stack) == 0); // Unbalanced stack is a bug

  cs->current_file = -1;
  return STATUS_SUCCEEDED;
}
