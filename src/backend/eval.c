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
#include <ast/symbols.h>
#include <backend/scope.h>
#include <frontend/bison/bison_parser.h>
#include <support/dyn_array.h>
#include <support/hash_map.h>
#include <support/logger.h>
#include <support/mem.h>
#include <support/state.h>
#include <support/str.h>
#include <support/types.h>
#include <support/util.h>

#include "eval.h"

typedef struct {
  Logger *logger;

  Stack *stack;
  dyn_arena *arena;
} Evaluator;

static CompilerState *cs = NULL;
static Evaluator *e = NULL;

/* -- Module init/shutdown -------------------------------------------------- */

void eval_init(CompilerState *compiler_state) {
  cs = compiler_state;

  e = new(Evaluator);
  e->logger = logger_create("Eval", stderr, LOG_DEBUG);
  e->arena = mem_dyn_arena_create(MEM_SIZE_AUTO);
}

void eval_shutdown() {
  if (!e) return;

  if (e->logger) logger_free(e->logger);
  if (e->arena) mem_dyn_arena_destroy(e->arena);
  free(e);

  e = NULL;
  cs = NULL;
}

/* -- Evaluation: utils ----------------------------------------------------- */

static void *eval_malloc(usize size) {
  return mem_dyn_arena_malloc(e->arena, size);
}

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

static bool _is_assignment(const Stmt *stmt) {
  return stmt->type == STMT_EXPR && stmt->expr->type == EXPR_ASSIGNMENT;
}

static Identifier _composable = {
  .lexeme = "$compose",
  .label = IDENTIFIER,
  .len = 8,
  .loc = {0}
};
static const Identifier *_composable_id() {
  return &_composable;
}

/* -- Evaluation: symbols --------------------------------------------------- */

static const FunctionDef *_resolve_function(
  const Symbols *s,
  const Identifier *name
) {
  const FunctionDef *function = NULL;
  StackEntry *st = st_current(e->stack);
  while (!function && st) {
    SymbolKey key = { .name = name->lexeme, .scope = st_scope(st).id };
    function = map_get(s->functions, &key);
    st = st_next(st);
  }

  assert(function != NULL);
  return function;
}


static Symbol *_resolve_symbol(const Symbols *s, const Identifier *name) {
  Symbol *symbol = NULL;
  StackEntry *st = st_current(e->stack);
  while (!symbol && st) {
    SymbolKey key = { .name = name->lexeme, .scope = st_scope(st).id };
    symbol = map_get(s->symbols, &key);
    st = st_next(st);
  }

  assert(symbol != NULL); // Undefined variable
  return symbol;
}

static const Value *eval_get_symbol(const Symbols *s, const Identifier *name) {
  return _resolve_symbol(s, name)->value;
}

static void eval_update_symbol(
  const Symbols *s,
  const Identifier *name,
  const Value *new_value
) {
  Symbol *sym = _resolve_symbol(s, name);
  assert(sym != NULL);
  sym->value = new_value;
}

/* -- Evaluation ------------------------------------------------------------ */

static const Value *eval_expr(const Symbols *s, const Expr *expr);
static const Value *eval_stmt(const Symbols *s, const Stmt *stmt);

static const Value *eval_range(const Symbols *s, const RangeLiteral *range) {
  const Value *start_v = eval_expr(s, range->start);
  const Value *end_v = eval_expr(s, range->end);
  assert(start_v->type == VAL_INT);
  assert(end_v->type == VAL_INT);

  eval_make(Range, range_v, {
    .start = start_v->integer,
    .end = end_v->integer,
  });
  eval_make(Value, value, { .type = VAL_RANGE, .range = range_v });
  return value;
}

static const Value *eval_list(const Symbols *s, const ListLiteral *list) {
  const Value *value = _list(list->len);
  for (u32 i = 0; i < list->len; i++) {
    value->list->items[i] = eval_expr(s, list->exprs[i]);
  }

  return value;
}

static const Value *eval_vector(const Symbols *s, const VectorLiteral *vec) {
  eval_make(List, list_v, {
    .len = vec->len,
    .items = eval_new_array(const Value *, vec->len)
  });

  const Value *item_val = eval_expr(s, vec->value);
  for (u32 i = 0; i < vec->len; i++) list_v->items[i] = item_val;

  eval_make(Value, value, { .type = VAL_LIST, .list = list_v });
  return value;
}

static const Value *eval_map(const Symbols *s, const MapLiteral *map) {
  Map *map_v = eval_new(Map);
  map_inita(map_v, eval_hash_value, eval_compare_values, eval_calloc, NULL);

  for (u32 i = 0; i < map->len; i++) {
    const Value *key_v = eval_expr(s, map->entries[i]->key);
    const Value *value_v = eval_expr(s, map->entries[i]->value);
    if (!map_insert(map_v, (Value *)key_v, (Value *)value_v)) {
      // TODO duplicate key in map literal error
      return _nil();
    }
  }

  eval_make(Value, value, { .type = VAL_MAP, .map = map_v });
  return value;
}

static const Value *eval_struct(const Symbols *s, const StructLiteral *struct_l) {
  Map *map_v = eval_new(Map);
  map_inita(map_v, eval_hash_value, eval_compare_values, eval_calloc, NULL);

  for (u32 i = 1; i < struct_l->len; i++) {
    const Value *key_v = _str_id(struct_l->fields[i]->name);
    const Value *value_v = eval_expr(s, struct_l->fields[i]->value);
    if (!map_insert(map_v, (Value *)key_v, (Value *)value_v)) {
      // TODO duplicate key in map literal error
      return _nil();
    }
  }

  eval_make(Value, value, { .type = VAL_MAP, .map = map_v });
  return value;
}

static const Value *eval_literal(const Symbols *s, const LiteralExpr *expr) {
  switch (expr->type) {
    case L_NIL: return _nil();
    case L_INTEGER: return _int(expr->integer->value);
    case L_FLOAT: return _float(expr->floating->value);
    case L_BOOL: return _bool(expr->boolean->value);
    case L_STRING: return _str(&expr->string->value);
    case L_RANGE: return eval_range(s, expr->range);
    case L_LIST: return eval_list(s, expr->list);
    case L_VECTOR: return eval_vector(s, expr->vec);
    case L_MAP: return eval_map(s, expr->map);
    case L_STRUCT: return eval_struct(s, expr->struct_literal);
    case L_ENUM: assert(false); // TODO
  }
}

static const Value *eval_list_idx(
  const Symbols *s,
  const Value *container,
  const IndexedVariable *var
) {
  const Value *value = NULL;
  u32 item_count = 0;
  for (u32 i = 0; i < var->index_count; i++) {
    const Value *idx = eval_expr(s, var->indices[i]);
    const Value *item_v = NULL;
    switch (idx->type) {
      case VAL_INT: {
        // TODO bounds check
        item_v = container->list->items[idx->integer];
        break;
      }
      case VAL_RANGE: {
        u32 start = idx->range->start;
        if (start < 0) start += container->list->len;
        u32 end = idx->range->end;
        if (end < 0) end += container->list->len;
        // TODO bounds check

        item_v = _list(end - start);
        for (u32 j = 0; j < end - start; j++) {
          item_v->list->items[j] = container->list->items[start + j];
        }
        break;
      }
      default: assert(false);
    }

    if (item_count == 0) {
      value = item_v;
      item_count = 1;
    } else if (item_count == 1) {
      const Value *first = value;
      value = _list(var->index_count);
      value->list->items[0] = first;
      value->list->items[item_count++] = item_v;
    } else {
      value->list->items[item_count++] = item_v;
    }
  }

  return value;
}

static const Value *eval_map_idx(
  const Symbols *s,
  const Value *container,
  const IndexedVariable *var
) {
  const Value *value = NULL;
  u32 item_count = 0;
  for (u32 i = 0; i < var->index_count; i++) {
    const Value *idx = eval_expr(s, var->indices[i]);
    const Value *item_v = map_get(container->map, (Value *)idx);
    if (item_count == 0) {
      value = item_v;
      item_count = 1;
    } else if (item_count == 1) {
      const Value *first = value;
      value = _list(var->index_count);
      value->list->items[0] = first;
      value->list->items[item_count++] = item_v;
    } else {
      value->list->items[item_count++] = item_v;
    }
  }

  return value;
}

static const Value *eval_variable(const Symbols *s, const VariableExpr *var) {
  switch (var->type) {
    case V_NAMED: return eval_get_symbol(s, var->named->name);
    case V_STRUCT_MEMBER: {
      const Value *struct_v = eval_variable(s, var->struct_member->struct_expr);
      assert(struct_v->type == VAL_MAP);
      const Value *key_v = _str_id(var->struct_member->name);
      return map_get(struct_v->map, (Value *)key_v);
    }
    case V_INDEX: {
      const Value *container_v = eval_variable(s, var->indexed->container);
      switch (container_v->type) {
        case VAL_LIST: return eval_list_idx(s, container_v, var->indexed);
        case VAL_MAP: return eval_map_idx(s, container_v, var->indexed);
        default: assert(false);
      }
    }
  }
}

static const Value *eval_unary(const Symbols *s, const UnaryExpr *expr) {
  const Value *inner = eval_expr(s, expr->expr);
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

static str eval_stringify(const Value *v) {
  switch (v->type) {
    case VAL_STRING: return *v->string;
    default: return str_from_cstring_a(eval_malloc, "(TODO stringify values)");
  }
}

static const Value *eval_string_concat(const str *l, const str *r) {
  str empty = { .ptr = "", .len = 0 };
  str *concat = eval_new(str);
  *concat = str_concat_a(eval_malloc, 2, empty, *l, *r);
  return _str(concat);
}

static const Value *eval_string_times(const str *l, i64 n) {
  str empty = { .ptr = "", .len = 0 };
  str *result = eval_new(str);
  *result = empty;
  for (u32 i = 0; i < n; i++) {
    *result = str_concat_a(eval_malloc, 2, empty, *result, *l);
  }
  return _str(result);
}

static const Value *eval_string_op(const str *l, const Value *r, TokenLabel op) {
  switch (op) {
    case PLUS: {
      str r_str = eval_stringify(r);
      return eval_string_concat(l, &r_str);
    }
    case STAR: {
      assert(r->type == VAL_INT);
      return eval_string_times(l, r->integer);
    }
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

static const Value *eval_binary(const Symbols *s, const BinaryExpr *expr) {
  const Value *left = eval_expr(s, expr->left);
  if (!left) return NULL;

  switch (expr->op) {
    case PLUS:
    case STAR: {
      const Value *right = eval_expr(s, expr->right);
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
      const Value *right = eval_expr(s, expr->right);
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
      const Value *right = eval_expr(s, expr->right);
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
      const Value *right = eval_expr(s, expr->right);
      if (!right) return NULL;

      return _bool(eval_compare_values(left, right));
    }
    case AND: {
      assert(left->type == VAL_BOOL);
      if (!left) return _bool(false);

      const Value *right = eval_expr(s, expr->right);
      if (!right) return NULL;

      assert(right->type == VAL_BOOL);
      return right;
    }
    case OR: {
      assert(left->type == VAL_BOOL);
      if (left) return _bool(true);

      const Value *right = eval_expr(s, expr->right);
      if (!right) return NULL;

      assert(right->type == VAL_BOOL);
      return right;
    }
    default: assert(false); // Unreachable
  }
}

static const Value *_block(const Symbols *s, const BlockExpr *expr) {
  const Value *value = NULL;
  u32 item_count = 0;

  for (u32 i = 0; i < expr->len; i++) {
    const Value *stmt_v = eval_stmt(s, expr->statements[i]);
    if (!stmt_v || _is_assignment(expr->statements[i])) continue;

    if (item_count == 0) {
      value = stmt_v;
      item_count++;
    } else if (item_count == 1) {
      const Value *lv = _list(expr->len);
      lv->list->items[0] = value;
      lv->list->items[item_count++] = stmt_v;
      value = lv;
    } else {
      value->list->items[item_count++] = stmt_v;
    }
  }

  if (item_count > 1) value->list->len = item_count;
  return value;
}

static const Value *eval_block(const Symbols *s, const BlockExpr *expr) {
  st_push(e->stack, expr->scope);
  const Value *value = _block(s, expr);
  st_pop(e->stack);
  return value;
}

static const Value *eval_assignment(const Symbols *s, const AssignmentExpr *a) {
  switch (a->left->type) {
    case V_NAMED: {
      const Value *r_value = eval_expr(s, a->right);
      eval_update_symbol(s, a->left->named->name, r_value);
      return r_value;
    }
    case V_STRUCT_MEMBER: {
      const Value *struct_v = eval_variable(
        s,
        a->left->struct_member->struct_expr
      );
      const Value *r_value = eval_expr(s, a->right);
      map_put(
        struct_v->map,
        (Value *)_str_id(a->left->struct_member->name),
        (Value *)r_value
      );
      return r_value;
    }
    case V_INDEX: {
      assert(a->left->indexed->index_count == 1);
      const Value *container_v = eval_variable(s, a->left->indexed->container);
      const Value *idx_v = eval_expr(s, a->left->indexed->indices[0]);
      const Value *r_value = eval_expr(s, a->right);
      switch (container_v->type) {
        case VAL_MAP: {
          map_put(container_v->map, (Value *)idx_v, (Value *)r_value);
          return r_value;
        }
        case VAL_LIST: {
          assert(idx_v->type == VAL_INT);
          // TODO bounds check
          container_v->list->items[idx_v->integer] = r_value;
          return r_value;
        }
        default: assert(false);
      }
    }
  }
}

static const Value *eval_if(const Symbols *s, const IfExpr *expr) {
  const Value *cond_v = eval_expr(s, expr->condition);
  assert(cond_v->type == VAL_BOOL);
  return cond_v->boolean
    ? eval_expr(s, expr->true_branch)
    : (expr->false_branch ? eval_expr(s, expr->false_branch) : NULL);
}

static const Value *eval_for(const Symbols *s, const ForExpr *expr) {
  const Value *iterable_v = eval_expr(s, expr->iterable);
  assert(iterable_v->type == VAL_LIST);

  st_push(e->stack, expr->body->block->scope);

  const Value *value = _list(iterable_v->list->len);
  for (u32 i = 0; i < iterable_v->list->len; i++) {
    eval_update_symbol(s, expr->var_id, iterable_v->list->items[i]);
    if (expr->idx_id) eval_update_symbol(s, expr->idx_id, _int(i));
    value->list->items[i] = _block(s, expr->body->block);
  }

  st_pop(e->stack);
  return value;
}

static const Value *eval_call(const Symbols *s, const FunctionCallExpr *call) {
  const FunctionDef *def = _resolve_function(s, call->name);

  const Value **arg_values = eval_new_array(const Value *, call->args_len);
  for (u32 i = 0; i < call->args_len; i++) {
    arg_values[i] = eval_expr(s, call->args[i]->value);
  }

  const Value *composable_v = NULL;
  if (call->composable) {
    composable_v = eval_expr(s, call->composable);
  }

  st_push(e->stack, def->scope);

  for (u32 i = 0; i < call->args_len; i++) {
    const Identifier *name = or_default(call->args[i]->name, def->params[i]->name);
    eval_update_symbol(s, name, arg_values[i]);
  }
  if (composable_v) {
    eval_update_symbol(s, _composable_id(), composable_v);
  }
  const Value *value = _block(s, def->body);

  st_pop(e->stack);
  return value;
}

static const Value *eval_compose(const Symbols *s, const Expr *expr) {
  return eval_get_symbol(s, _composable_id());
}

static const Value *eval_expr(const Symbols *s, const Expr *expr) {
  switch (expr->type) {
    case EXPR_LITERAL: return eval_literal(s, expr->literal);
    case EXPR_VARIABLE: return eval_variable(s, expr->variable);
    case EXPR_UNARY: return eval_unary(s, expr->unary);
    case EXPR_BINARY: return eval_binary(s, expr->binary);
    case EXPR_ASSIGNMENT: return eval_assignment(s, expr->assignment);
    case EXPR_IF: return eval_if(s, expr->if_expr);
    case EXPR_FOR: return eval_for(s, expr->for_expr);
    case EXPR_FUNCTION_CALL: return eval_call(s, expr->function_call);
    case EXPR_BLOCK: return eval_block(s, expr->block);
    case EXPR_GROUP: return eval_expr(s, expr->group->inner_expr);
    case EXPR_COMPOSE: return eval_compose(s, expr);
  }
  assert(false); // perish
}

static const Value *eval_decl(const Symbols *s, const DeclarationStmt *decl) {
  const Value *r_value = eval_expr(s, decl->right);
  if (decl->left_count == 1) {
    eval_update_symbol(s, decl->left[0], r_value);
  } else {
    assert(r_value->type == VAL_LIST);
    // TODO check left count <= right len
    for (u32 i = 0; i < decl->left_count; i++) {
      eval_update_symbol(s, decl->left[i], r_value->list->items[i]);
    }
  }
  return NULL;
}

static const Value *eval_const(const Symbols *s, const ConstantStmt *constant) {
  const Value *r_value = eval_expr(s, constant->right);
  eval_update_symbol(s, constant->left, r_value);
  return NULL;
}

static const Value *eval_stmt(const Symbols *s, const Stmt *stmt) {
  switch (stmt->type) {
    case STMT_EXPR: return eval_expr(s, stmt->expr);
    case STMT_DECLARATION: return eval_decl(s, stmt->decl);
    case STMT_CONSTANT: return eval_const(s, stmt->constant);
    case STMT_TYPE_ALIAS: return NULL;
    case STMT_EXPORT: return NULL;
    case STMT_FUNCTION_DEF: return NULL;
    case STMT_PARSE_ERROR: assert(false); // Unreachable
  }
}

static const Value *eval_program(const Symbols *s, const Program *program) {
  st_push(e->stack, program->scope);

  const Value *value = NULL;
  if (program->len == 1) {
    value = eval_stmt(s, program->statements[0]);
  } else {
    u32 item_count = 0;
    for (u32 i = 0; i < program->len; i++) {
      const Value *stmt_v = eval_stmt(s, program->statements[i]);
      if (!stmt_v || _is_assignment(program->statements[i])) continue;

      if (item_count == 0) {
        value = stmt_v;
        item_count++;
      } else if (item_count == 1) {
        const Value *lv = _list(program->len);
        lv->list->items[0] = value;
        lv->list->items[item_count++] = stmt_v;
        value = lv;
      } else {
        value->list->items[item_count++] = stmt_v;
      }
    }
    if (item_count > 1) value->list->len = item_count;
  }

  st_pop(e->stack);
  return value;
}

CompilationStatus eval_evaluate(i32 file_idx) {
  cs->current_file = file_idx;

  Program *ast = cs->asts[file_idx];
  assert(ast != NULL); // If we got to this point without an AST we have a bug

  Symbols *sym = cs->symbol_data[file_idx];
  assert(sym != NULL); // If we got to this point without a symbol table we have a bug

  e->stack = st_begin();
  // mem_dyn_arena_free_all(sa->tc_arena);
  const Value *val = eval_program(sym, ast);
  st_end(e->stack);

  cs->current_file = -1;
  return STATUS_SUCCEEDED;
}
