#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <ast/ast.h>
#include <ast/symbols.h>
#include <error/error.h>
#include <frontend/bison/bison_parser.h>
#include <support/dyn_array.h>
#include <support/hash_map.h>
#include <support/logger.h>
#include <support/state.h>
#include <support/types.h>
#include <support/util.h>

#include "static_analysis.h"

typedef struct {
  Logger *sym_logger;
  Logger *tc_logger;
  dyn_array *stack;
  u32 next_scope_id;
} StaticAnalyzer;

static CompilerState *cs = NULL;
static StaticAnalyzer *sa = NULL;

/* -- Scope ----------------------------------------------------------------- */

static u32 _new_scope() { return sa->next_scope_id++; }
static u32 _push_scope(u32 scope) {
  dyn_array_push(sa->stack, &scope);
  return scope;
}

static void _pop_scope() {
  dyn_array_remove_unordered(sa->stack, dyn_array_size(sa->stack) - 1);
}

static u32 _peek_scope() {
  return *(u32 *)dyn_array_get(sa->stack, dyn_array_size(sa->stack) - 1);
}

/* -- Module init/shutdown -------------------------------------------------- */

void sa_init(CompilerState *compiler_state) {
  cs = compiler_state;

  sa = new(StaticAnalyzer);
  sa->next_scope_id = 0;
  sa->stack = new_dyn_array(u32);
  sa->sym_logger = logger_create("Symbols", stderr,
    cs->options.symbols_log_level
  );
  sa->tc_logger = logger_create("Typecheck", stderr,
    cs->options.typecheck_log_level
  );
}

void sa_shutdown() {
  if (!sa) return;

  if (sa->stack) dyn_array_free(sa->stack);
  if (sa->sym_logger) logger_free(sa->sym_logger);
  if (sa->tc_logger) logger_free(sa->tc_logger);
  free(sa);

  sa = NULL;
  cs = NULL;
}

/* -- Static symbol pass ---------------------------------------------------- */

static void _static_tables_stmt(Symbols *s, Stmt *stmt);
static void _static_tables_expr(Symbols *s, Expr *expr);
static void _static_tables_block(Symbols *s, BlockExpr *block);
static void _static_tables_literal(Symbols *s, LiteralExpr *literal);
static void _static_tables_variable(Symbols *s, VariableExpr *var);

static void _static_tables_add_type(Symbols *s, TypeAliasStmt *type_alias) {
  SymbolKey key = { .name = type_alias->alias->lexeme, .scope = _peek_scope() };
  logger_log(sa->sym_logger, LOG_DEBUG,
    "Scope %u: Type %s",
    key.scope, key.name
  );

  if (!map_insert(s->types, &key, type_alias)) {
    logger_log(sa->sym_logger, LOG_ERROR,
      "Scope %u: Redefinition of type %s",
      key.scope, key.name
    );

    TypeAliasStmt *prev = map_get(s->types, &key);
    RedefError *error = new(RedefError);
    *error = (RedefError){
      .object_type = "type",
      .object_name = strdup(key.name),
      .previous_definition = prev->meta.loc
    };
    err_report_redef(error, &type_alias->meta.loc, ERR_ERROR);
  }
}

static void _static_tables_add_function(Symbols *s, FunctionDef *function) {
  _static_tables_block(s, function->body);

  SymbolKey key = { .name = function->name->lexeme, .scope = _peek_scope() };
  logger_log(sa->sym_logger, LOG_DEBUG,
    "Scope %u: Function %s",
    key.scope, key.name
  );

  if (!map_insert(s->functions, &key, function)) {
    logger_log(sa->sym_logger, LOG_ERROR,
      "Scope %u: Redefinition of function %s",
      key.scope, key.name
    );

    FunctionDef *prev = map_get(s->functions, &key);
    RedefError *error = new(RedefError);
    *error = (RedefError){
      .object_type = "function",
      .object_name = strdup(key.name),
      .previous_definition = prev->meta.loc
    };
    err_report_redef(error, &function->meta.loc, ERR_ERROR);
  }
}

static void _static_tables_block(Symbols *s, BlockExpr *block) {
  block->scope = _push_scope(_new_scope());
  logger_log(sa->sym_logger, LOG_DEBUG,
    ">>> Scope %u [%d:%d]",
    block->scope, block->meta.loc.first_line, block->meta.loc.first_column
  );
  for (u32 i = 0; i < block->len; i++) {
    _static_tables_stmt(s, block->statements[i]);
  }
  logger_log(sa->sym_logger, LOG_DEBUG,
    "<<< Scope %u [%d:%d]",
    block->scope, block->meta.loc.last_line, block->meta.loc.last_column
  );
  _pop_scope();
}

static void _static_tables_literal(Symbols *s, LiteralExpr *literal) {
  switch (literal->type) {
    case L_LIST: {
      for (u32 i = 0; i < literal->list->len; i++) {
        _static_tables_expr(s, literal->list->exprs[i]);
      }
      break;
    }
    case L_LIST_SHORT: {
      _static_tables_expr(s, literal->short_list->value);
      break;
    }
    case L_MAP: {
      for (u32 i = 0; i < literal->map->len; i++) {
        _static_tables_expr(s, literal->map->entries[i]->key);
        _static_tables_expr(s, literal->map->entries[i]->value);
      }
      break;
    }
    case L_STRUCT: {
      for (u32 i = 0; i < literal->struct_literal->len; i++) {
        _static_tables_expr(s, literal->struct_literal->fields[i]->value);
      }
      break;
    }
    case L_RANGE: {
      if (literal->range->start) _static_tables_expr(s, literal->range->start);
      if (literal->range->end) _static_tables_expr(s, literal->range->end);
      break;
    }
    default: break;
  }
}

static void _static_tables_variable(Symbols *s, VariableExpr *var) {
  switch (var->type) {
    case V_INDEX: {
      _static_tables_variable(s, var->indexed->container);
      for (u32 i = 0; i < var->indexed->index_count; i++) {
        _static_tables_expr(s, var->indexed->indices[i]);
      }
      break;
    }
    case V_STRUCT_MEMBER: {
      _static_tables_variable(s, var->struct_member->struct_expr);
      break;
    }
    case V_NAMED: break;
  }
}

static void _static_tables_expr(Symbols *s, Expr *expr) {
  switch (expr->type) {
    case EXPR_BLOCK: _static_tables_block(s, expr->block); break;
    case EXPR_GROUP: _static_tables_expr(s, expr->group->inner_expr); break;
    case EXPR_UNARY: _static_tables_expr(s, expr->unary->expr); break;
    case EXPR_LITERAL: _static_tables_literal(s, expr->literal); break;
    case EXPR_VARIABLE: _static_tables_variable(s, expr->variable); break;
    case EXPR_ENV: _static_tables_expr(s, expr->env->default_value); break;
    case EXPR_FUNCTION_CALL: {
      for (u32 i = 0; i < expr->function_call->args_len; i++) {
        _static_tables_expr(s, expr->function_call->args[i]->value);
      }
      if (expr->function_call->composable) {
        _static_tables_expr(s, expr->function_call->composable);
      }
      break;
    }
    case EXPR_ASSIGNMENT: {
      _static_tables_variable(s, expr->assignment->left);
      _static_tables_expr(s, expr->assignment->right);
      break;
    }
    case EXPR_BINARY: {
      _static_tables_expr(s, expr->binary->left);
      _static_tables_expr(s, expr->binary->right);
      break;
    }
    case EXPR_FOR: {
      _static_tables_expr(s, expr->for_expr->iterable);
      _static_tables_expr(s, expr->for_expr->body);
      break;
    }
    case EXPR_IF: {
      _static_tables_expr(s, expr->if_expr->condition);
      _static_tables_expr(s, expr->if_expr->true_branch);
      if (expr->if_expr->false_branch) {
        _static_tables_expr(s, expr->if_expr->false_branch);
      }
      break;
    }
    case EXPR_IMPORT: break;
  }
}

static void _static_tables_stmt(Symbols *s, Stmt *stmt) {
  switch (stmt->type) {
    case STMT_TYPE_ALIAS: _static_tables_add_type(s, stmt->type_alias); break;
    case STMT_FUNCTION_DEF: _static_tables_add_function(s, stmt->function_def); break;
    case STMT_EXPR: _static_tables_expr(s, stmt->expr); break;
    default: break;
  }
}

static void _build_static_tables_ast(Symbols *s, Program *ast) {
  ast->scope = _push_scope(_new_scope());
  logger_log(sa->sym_logger, LOG_DEBUG, ">>> Scope %u", ast->scope);
  for (u32 i = 0; i < ast->len; i++) {
    _static_tables_stmt(s, ast->statements[i]);
  }
  logger_log(sa->sym_logger, LOG_DEBUG, "<<< Scope %u", ast->scope);
  _pop_scope();
}

CompilationStatus sa_build_static_tables(i32 file_idx) {
  cs->current_file = file_idx;

  Program *ast = cs->asts[file_idx];
  assert(ast != NULL); // If we got to this point without an AST we have a bug

  Symbols *sym = new(Symbols);
  sym->types = new(TypeTable);
  sym->functions = new(FunctionTable);
  map_init(sym->types, sk_hash, sk_compare);
  map_manage_keys(sym->types, sk_clone, sk_free);
  map_init(sym->functions, sk_hash, sk_compare);
  map_manage_keys(sym->functions, sk_clone, sk_free);
  cs->symbol_data[file_idx] = sym;

  dyn_array_clear(sa->stack);
  _build_static_tables_ast(sym, ast);
  assert(dyn_array_size(sa->stack) == 0); // Unbalanced stack is a bug

  cs->current_file = -1;
  return STATUS_SUCCEEDED;
}

/* -- Type checking: utils -------------------------------------------------- */

#define tc_new(T) new(T)
#define tc_new_array(T, n) new_array(T, n)

// C is for Cursed macro dark arts
#define tc_make(T, name, ...)               \
  T *name = tc_new(T);                      \
  *name = (T)__VA_ARGS__

static Type *_builtin_type(BuiltinType builtin) {
  tc_make(Type, type, { .type = T_BUILTIN, .builtin = builtin, .meta = {0} });
  return type;
}

static Type *_list_of(Type *item_type) {
  tc_make(ListType, list, { .item_type = item_type, .meta = {0} });
  tc_make(Type, type, { .type = T_LIST, .list = list, .meta = {0} });
  return type;
}

static Type *_tuple_of(Type **types, u32 len) {
  tc_make(TupleType, tuple, {
    .types = types,
    .len = len,
    .meta = {0},
  });
  tc_make(Type, type, { .type = T_TUPLE, .tuple = tuple, .meta = {0} });
  return type;
}

static Type *_short_tuple_of(Type *item_type, u32 count) {
  tc_make(ShortTupleType, st, {
    .type = item_type,
    .count = count,
    .meta = {0},
  });
  tc_make(Type, type, { .type = T_TUPLE_SHORT, .short_tuple = st, .meta = {0} });
  return type;
}

static Type *_map_of(Type *key_type, Type *value_type) {
  tc_make(MapType, map, {
    .key_type = key_type,
    .value_type = value_type,
    .meta = {0}
  });
  tc_make(Type, type, { .type = T_MAP, .map = map, .meta = {0} });
  return type;
}

/* -- Type checking: compare ------------------------------------------------ */

static bool _is_builtin_type(Type *type, BuiltinType builtin) {
  return type->type == T_BUILTIN && type->builtin == builtin;
}

static bool _is_numeric_type(Type *type) {
  return type->type == T_BUILTIN
    && (type->builtin == T_INT || type->builtin == T_FLOAT);
}

static bool _is_vector_type(Type *type) {
  return type->type == T_TUPLE_SHORT
    && _is_numeric_type(type->short_tuple->type);
}

static Type *_upgrade_numeric_type(BuiltinType l, BuiltinType r) {
  if (l == T_INT && r == T_INT) return _builtin_type(T_INT);
  return _builtin_type(T_FLOAT);
}

static Type *_resolve_named_type(Symbols *s, NamedType *named) {
  SymbolKey key = { .name = named->name->lexeme, .scope = _peek_scope() };
  TypeAliasStmt *alias = map_get(s->types, &key);
  if (!alias) {
    UndefError *error = new(UndefError);
    *error = (UndefError){
      .object_type = "type",
      .object_name = strdup(key.name),
    };
    err_report_undef(error, &named->meta.loc, ERR_ERROR);
    return NULL;
  }

  return alias->type;
}

static bool tc_compare_types(const Type *a, const Type *b) {
  // If the types are literally the same object they are obviously equal
  if (a == b) return true;

  // TODO
}

static bool tc_accepts_type(const Type *expected, const Type *actual) {
  // If the types are literally the same object they are obviously equal
  if (actual == expected) return true;

  // TODO
}

static bool tc_compatible_types(const Type *a, const Type *b) {
  return tc_accepts_type(a, b) || tc_accepts_type(b, a);
}

/* -- Type checking: check -------------------------------------------------- */

static Type *tc_check_expr(Symbols *s, const Expr *expr);
static Type *tc_check_stmt(Symbols *s, const Stmt *stmt);

static Type *_list_type(Symbols *s, ListLiteral *list) {
  Type **types = tc_new_array(Type *, list->len);
  for (u32 i = 0; i < list->len; i++) {
    types[i] = tc_check_expr(s, list->exprs[i]);
  }

  tc_make(TupleType, tuple, { .types = types, .len = list->len, .meta = {0} });
  tc_make(Type, type, { .type = T_TUPLE, .tuple = tuple, .meta = {0} });
  return type;
}

static Type *_short_list_type(Symbols *s, ShortListLiteral *short_list) {
  Type *item_type = tc_check_expr(s, short_list->value);

  tc_make(ShortTupleType, short_tuple, {
    .type = item_type,
    .count = short_list->count,
    .meta = {0}
  });
  tc_make(Type, type, {
    .type = T_TUPLE_SHORT,
      .short_tuple = short_tuple,
      .meta = {0}
  });
  return type;
}

static bool _is_valid_map_key_type(const Type *type) {
  switch (type->type) {
    case T_BUILTIN:       return type->builtin != T_NIL && type->builtin != T_VOID;
    case T_NAMED:         assert(false); // Named types should not make it this far!
    case T_UNION:         return false;
    case T_MAP:           return false;
    case T_LIST:          return false;
    case T_RANGE:         return false;
    case T_ENUM:          return true;
    case T_TUPLE_SHORT:   return _is_valid_map_key_type(type->short_tuple->type);
    case T_STRUCT: {
      for (u32 i = 0; i < type->struct_type->len; i++) {
        if (!_is_valid_map_key_type(type->struct_type->fields[i]->type)) return false;
      }
      return true;
    }
    case T_TUPLE: {
      for (u32 i = 0; i < type->tuple->len; i++) {
        if (!_is_valid_map_key_type(type->tuple->types[i])) return false;
      }
      return true;
    }
  }
}

static bool _union_contains_type(const Type *u, const Type *type) {
  assert(u->type == T_UNION);
  for (u32 i = 0; i < u->union_type->len; i++) {
    if (tc_accepts_type(u->union_type->types[i], type)) return true;
  }
  return false;
}

static Type *_add_to_union_type(Type *value_type, Type *new_type) {
  if (!value_type || !new_type) return NULL;

  if (value_type->type == T_UNION) {
    if (_union_contains_type(value_type, new_type)) {
      return value_type;
    }

    UnionType *u = value_type->union_type;
    u->len++;
    u->types = realloc(u->types, u->len * sizeof(Type *)); // FIXME leaks and stinks!!
    return value_type;
  } else {
    if (tc_accepts_type(value_type, new_type)) {
      return value_type;
    }

    tc_make(UnionType, union_type, {
      .len = 2,
      .types = tc_new_array(Type *, 2),
      .meta = {0},
    });
    union_type->types[0] = value_type;
    union_type->types[1] = new_type;

    tc_make(Type, type, { .type = T_UNION, .union_type = union_type, .meta = {0} });
    return type;
  }
}

static Type *_map_type(Symbols *s, MapLiteral *map) {
  Type *key_type = tc_check_expr(s, map->entries[0]->key);
  Type *value_type = tc_check_expr(s, map->entries[0]->value);
  bool valid = true;

  if (!_is_valid_map_key_type(key_type)) {
    TypeError *error = new(TypeError);
    *error = (TypeError){ .message = "invalid key type" };
    err_report_type(error, &map->entries[0]->key->meta.loc, ERR_ERROR);
    valid = false;
  }

  for (u32 i = 1; i < map->len; i++) {
    Type *k = tc_check_expr(s, map->entries[i]->key);
    Type *v = tc_check_expr(s, map->entries[i]->value);

    if (!tc_compare_types(key_type, k)) {
      TypeError *error = new(TypeError);
      *error = (TypeError){ .message = "inconsistent key type" };
      err_report_type(error, &map->entries[i]->key->meta.loc, ERR_ERROR);
      valid = false;
    }

    value_type = _add_to_union_type(value_type, v);
  }
  if (!valid) return NULL;

  tc_make(MapType, map_type, {
    .key_type = key_type,
    .value_type = value_type,
    .meta = {0}
  });
  tc_make(Type, type, { .type = T_MAP, .map = map_type, .meta = {0} });
  return type;
}

static Type *_struct_type(Symbols *s, StructLiteral *struct_l) {
  StructField **fields = tc_new_array(StructField *, struct_l->len);
  for (u32 i = 0; i < struct_l->len; i++) {
    StructLiteralField *lf = struct_l->fields[i];
    fields[i] = tc_new(StructField);
    *fields[i] = (StructField) {
      .name = ast_clone_identifier(lf->name),
      .type = tc_check_expr(s, lf->value),
    };
  }

  tc_make(StructType, struct_type, {
    .len = struct_l->len,
    .fields = fields,
    .meta = {0}
  });
  tc_make(Type, type, { .type = T_STRUCT, .struct_type = struct_type, .meta = {0} });
  return type;
}

static Type *_enum_type(EnumLiteral *enum_l) {
  return enum_l->type;
}

static Type *_range_type(Symbols *s, RangeLiteral *range) {
  Type *start_type = tc_check_expr(s, range->start);
  Type *end_type = tc_check_expr(s, range->end);
  if (!_is_builtin_type(start_type, T_INT) || !_is_builtin_type(end_type, T_INT)) {
    TypeError *error = new(TypeError);
    *error = (TypeError){
      .message = "range start/end expressions must have integer type"
    };
    err_report_type(error, &range->meta.loc, ERR_ERROR);

    return NULL;
  }

  tc_make(Type, type, { .type = T_RANGE, .meta = {0} });
  return type;
}

static Type *_literal_type(Symbols *s, LiteralExpr *expr) {
  switch (expr->type) {
    case L_INTEGER:     return _builtin_type(T_INT);
    case L_FLOAT:       return _builtin_type(T_FLOAT);
    case L_STRING:      return _builtin_type(T_STRING);
    case L_BOOL:        return _builtin_type(T_BOOL);
    case L_NIL:         return _builtin_type(T_NIL);
    case L_LIST:        return _list_type(s, expr->list);
    case L_LIST_SHORT:  return _short_list_type(s, expr->short_list);
    case L_MAP:         return _map_type(s, expr->map);
    case L_STRUCT:      return _struct_type(s, expr->struct_literal);
    case L_ENUM:        return _enum_type(expr->enum_literal);
    case L_RANGE:       return _range_type(s, expr->range);
  }
}

static Type *_struct_member_type(const StructType *s, const char *field) {
  for (u32 i = 0; i < s->len; i++) {
    if (strcmp(s->fields[i]->name->lexeme, field) == 0) {
      return s->fields[i]->type;
    }
  }

  // TODO error: field name not in struct
  return NULL;
}

static Type *_indexed_list_item_type(Symbols *s, Type *item_type, Expr *index) {
  Type *index_type = tc_check_expr(s, index);
  switch (index_type->type) {
    case T_RANGE: return _list_of(item_type);
    case T_BUILTIN: {
      if (index_type->builtin == T_INT) {
        return item_type;
      } else {
        // TODO bad index type error
        return NULL;
      }
    }
    default: {
      // TODO bad index type error
      return NULL;
    }
  }
}

static Type *_indexed_list_type(Symbols *s, ListType *list, IndexedVariable *var) {
  if (var->index_count == 1) {
    return _indexed_list_item_type(s, list->item_type, var->indices[0]);
  } else {
    Type **types = tc_new_array(Type *, var->index_count);
    bool valid = true;
    for (u32 i = 0; i < var->index_count; i++) {
      types[i] = _indexed_list_item_type(s, list->item_type, var->indices[i]);
      if (!types[i]) valid = false;
    }

    return valid ? _tuple_of(types, var->index_count) : NULL;
  }
}

static Type *_indexed_tuple_item_type(Symbols *s, TupleType *tuple, Expr *index) {
  Type *index_type = tc_check_expr(s, index);
  if (!_is_builtin_type(index_type, T_INT)) {
    // TODO bad index type error
    return NULL;
  }

  // If the index is an integer literal, we can infer the actual item type since
  // the value is known at time of type checking
  if (index->type == EXPR_LITERAL && index->literal->type == L_INTEGER) {
    i64 idx = index->literal->integer->value;
    if (idx < 0) idx += tuple->len;
    if (idx < 0 || idx >= tuple->len) {
      // TODO tuple index out of bounds error
      return NULL;
    }
    return tuple->types[idx];
  }

  // For any other expression we'd need to evaluate it to know which tuple item
  // it references, so we just return a union of all types in the tuple
  Type *value_type = tuple->types[0];
  for (u32 i = 1; i < tuple->len; i++) {
    value_type = _add_to_union_type(value_type, tuple->types[i]);
  }
  return value_type;
}

static Type *_indexed_tuple_type(Symbols *s, TupleType *tuple, IndexedVariable *var) {
  if (var->index_count == 1) {
    return _indexed_tuple_item_type(s, tuple, var->indices[0]);
  } else {
    Type **types = tc_new_array(Type *, var->index_count);
    bool valid = true;
    for (u32 i = 0; i < var->index_count; i++) {
      types[i] = _indexed_tuple_item_type(s, tuple, var->indices[i]);
      if (!types[i]) valid = false;
    }

    return valid ? _tuple_of(types, var->index_count) : NULL;
  }
}

static Type *_indexed_short_tuple_type(Symbols *s, ShortTupleType *short_tuple, IndexedVariable *var) {
  if (var->index_count == 1) {
    Type *index_type = tc_check_expr(s, var->indices[0]);
    if (!_is_builtin_type(index_type, T_INT)) {
      // TODO bad index type error
      return NULL;
    }
    return short_tuple->type;
  } else {
    bool valid = true;
    for (u32 i = 0; i < var->index_count; i++) {
      Type *index_type = tc_check_expr(s, var->indices[i]);
      if (!_is_builtin_type(index_type, T_INT)) {
        // TODO bad index type error
        valid = false;
      }
    }

    return valid ? _short_tuple_of(short_tuple->type, var->index_count) : NULL;
  }
}

static Type *_indexed_map_type(Symbols *s, MapType *map, IndexedVariable *var) {
  if (var->index_count == 1) {
    if (!tc_accepts_type(map->key_type, tc_check_expr(s, var->indices[0]))) {
      // TODO bad index type error
      return NULL;
    }
    return map->value_type;
  } else {
    bool valid = true;
    for (u32 i = 0; i < var->index_count; i++) {
      if (!tc_accepts_type(map->key_type, tc_check_expr(s, var->indices[i]))) {
        // TODO bad index type error
        valid = false;
      }
    }

    return valid ? _short_tuple_of(map->value_type, var->index_count) : NULL;
  }
}

static Type *_variable_type(Symbols *s, VariableExpr *var) {
  switch (var->type) {
    case V_NAMED: {
      SymbolKey key = { .name = var->named->name->lexeme, .scope = _peek_scope() };
      return map_get(s->symbols, &key)->type;
    }
    case V_INDEX: {
      Type *container = _variable_type(s, var->indexed->container);
      switch (container->type) {
        case T_LIST: return _indexed_list_type(s, container->list, var->indexed);
        case T_TUPLE: return _indexed_tuple_type(s, container->tuple, var->indexed);
        case T_TUPLE_SHORT: return _indexed_short_tuple_type(s, container->short_tuple, var->indexed);
        case T_MAP: return _indexed_map_type(s, container->map, var->indexed);
        default: {
          // TODO error: not an indexable type
          return NULL;
        }
      }
    }
    case V_STRUCT_MEMBER: {
      Type *container = _variable_type(s, var->struct_member->struct_expr);
      if (container->type != T_STRUCT) {
        // TODO error: trying to access struct member of non-struct
        return NULL;
      }
      return _struct_member_type(container->struct_type, var->struct_member->name->lexeme);
    }
  }
}

static Type *_unary_type(Symbols *s, UnaryExpr *expr) {
  switch (expr->op) {
    case BANG:
    case NOT: {
      Type *expr_type = tc_check_expr(s, expr->expr);
      if (!_is_builtin_type(expr_type, T_BOOL)) {
        // TODO invalid type for operation
        return NULL;
      }
      return expr_type;
    }
    case MINUS: {
      Type *expr_type = tc_check_expr(s, expr->expr);
      if (!_is_builtin_type(expr_type, T_INT) && !_is_builtin_type(expr_type, T_FLOAT)) {
        // TODO invalid type for operation
        return NULL;
      }
      return expr_type;
    }
    default: assert(false); // Unreachable
  }
}

static Type *_arithmetic_numeric_type(Symbols *s, Type *l, Type *r, TokenLabel op) {
  if (_is_numeric_type(r)) {
    return _upgrade_numeric_type(l->builtin, r->builtin);
  } else if (_is_vector_type(r)) {
    return _short_tuple_of(
      _upgrade_numeric_type(l->builtin, r->short_tuple->type->builtin),
      r->short_tuple->count
    );
  } else {
    // TODO wrong type for operand
    return NULL;
  }
}

static Type *_arithmetic_vector_type(Symbols *s, Type *l, Type *r, TokenLabel op) {
  if (_is_vector_type(r)) {
    if (l->short_tuple->count != r->short_tuple->count) {
      // TODO length mismatch
      return NULL;
    }
    Type *num_t = _upgrade_numeric_type(
      l->short_tuple->type->builtin,
      r->short_tuple->type->builtin
    );
    return _short_tuple_of(num_t, l->short_tuple->count);
  } else if (_is_numeric_type(r)) {
    return _short_tuple_of(
      _upgrade_numeric_type(l->short_tuple->type->builtin, r->builtin),
      l->short_tuple->count
    );
  } else {
    // TODO wrong type for operand
    return NULL;
  }
}

static Type *_binary_type(Symbols *s, BinaryExpr *expr) {
  Type *l_type = tc_check_expr(s, expr->left);
  Type *r_type = tc_check_expr(s, expr->right);

  switch (expr->op) {
    case PLUS:
    case STAR: {
      if (_is_numeric_type(l_type)) {
        return _arithmetic_numeric_type(s, l_type, r_type, expr->op);
      } else if (_is_vector_type(l_type)) {
        return _arithmetic_vector_type(s, l_type, r_type, expr->op);
      } else if (_is_builtin_type(l_type, T_STRING)) {
        if (expr->op == PLUS) {
          if (r_type != NULL) {
            return l_type;
          } else {
            // TODO invalid rhs
            return NULL;
          }
        } else if (expr->op == STAR) {
          if (_is_builtin_type(r_type, T_INT)) {
            return l_type;
          } else {
            // TODO invalid rhs
            return NULL;
          }
        }
      } else {
        // TODO invalid type for operator
        return NULL;
      }
    }
    case MINUS:
    case SLASH: {
      if (_is_numeric_type(l_type)) {
        return _arithmetic_numeric_type(s, l_type, r_type, expr->op);
      } else if (_is_vector_type(l_type)) {
        return _arithmetic_vector_type(s, l_type, r_type, expr->op);
      } else {
        // TODO invalid type for operator
        return NULL;
      }
    }
    case GREATER_EQUAL:
    case GREATER:
    case LESS_EQUAL:
    case LESS: {
      if (_is_numeric_type(l_type)) {
        if (_is_numeric_type(r_type)) {
          return _upgrade_numeric_type(l_type->builtin, r_type->builtin);
        } else {
          // TODO type mismatch
          return NULL;
        }
      } else if (_is_builtin_type(l_type, T_STRING)) {
        if (_is_builtin_type(r_type, T_STRING)) {
          return l_type;
        } else {
          // TODO type mismatch
          return NULL;
        }
      } else {
        // TODO non ordered type
        return NULL;
      }
    }
    case EQUAL_EQUAL:
    case BANG_EQUAL: {
      if (!tc_compatible_types(l_type, r_type)) {
        // TODO comparison of incompatible types
        return NULL;
      }
    }
    case AND:
    case OR: {
      bool left_valid = _is_builtin_type(l_type, T_BOOL);
      bool right_valid = _is_builtin_type(r_type, T_BOOL);
      if (!left_valid) {
        // TODO invalid type error
        return NULL;
      }
      if (!right_valid) {
        // TODO invalid type error
        return NULL;
      }
      return (left_valid && right_valid) ? l_type : NULL;
    }
    default: assert(false); // Unreachable
  }
}

static Type *_assignment_type(Symbols *s, const AssignmentExpr *expr) {
  Type *lhs_type = _variable_type(s, expr->left);
  Type *rhs_type = tc_check_expr(s, expr->right);

  if (tc_accepts_type(lhs_type, rhs_type)) return lhs_type;
  // TODO type error: incompatible types
  return NULL;
}

static Type *_if_expr_type(Symbols *s, const IfExpr *expr) {
  Type *condition_type = tc_check_expr(s, expr->condition);
  Type *true_type = tc_check_expr(s, expr->true_branch);
  bool valid = true;

  if (expr->false_branch) {
    Type *false_type = tc_check_expr(s, expr->false_branch);
    // TODO compare types in a somewhat more forgiving way
    if (!tc_compare_types(true_type, false_type)) {
      // TODO incompatible else type error
      valid = false;
    }
  }

  if (!_is_builtin_type(condition_type, T_BOOL)) {
    // TODO condition must resolve to boolean type
    valid = false;
  }

  return valid ? true_type : NULL;
}

static Type *_for_expr_type(Symbols *s, const ForExpr *expr) {
  Type *iterable_type = tc_check_expr(s, expr->iterable);
  Type *body_type = tc_check_expr(s, expr->body);

  switch (iterable_type->type) {
    case T_LIST: {
      return _list_of(body_type);
    }
    case T_TUPLE_SHORT: {
      return _short_tuple_of(body_type, iterable_type->short_tuple->count);
    }
    default: {
      // TODO type error non iterable type in for expression
      return NULL;
    }
  }
}

static u32 _find_function_param(const FunctionDef *def, const Identifier *name) {
  for (u32 j = 0; j < def->params_len; j++) {
    if (ast_compare_identifiers(def->params[j]->name, name)) return j;
  }
  return -1;
}

static bool _check_function_args(Symbols *s, const FunctionCallExpr *expr, const FunctionDef *def) {
  if (expr->args_len > def->params_len) {
    // TODO args count error
    return false;
  }

  bool positional_args_allowed = true;
  bool valid = true;
  bool *params_set = tc_new_array(bool, def->params_len);
  for (u32 i = 0; i < expr->args_len; i++) {
    Argument *arg = expr->args[i];
    i32 param_idx = -1;

    if (arg->name) {
      positional_args_allowed = false; // Positional args must go before named
      param_idx = _find_function_param(def, arg->name);
      if (param_idx == -1) {
        // TODO invalid parameter name error
        valid = false; continue;
      }
    } else {
      if (!positional_args_allowed) {
        // TODO positional arg after named error
        valid = false; continue;
      }
      param_idx = i;
    }

    Parameter *param = def->params[param_idx];
    if (!tc_accepts_type(param->type, tc_check_expr(s, arg->value))) {
      // TODO parameter type error
      valid = false;
    } else {
      params_set[param_idx] = true;
    }
  }

  // Ensure all the required parameters are actually being passed
  for (u32 i = 0; i < def->params_len; i++) {
    if (!params_set[i] && !def->params[i]->default_value) {
      // TODO required parameter not set error
      valid = false;
    }
  }

  return valid;
}

static Type *_function_call_type(Symbols *s, const FunctionCallExpr *expr) {
  SymbolKey key = { .name = expr->name->lexeme, .scope = _peek_scope() };
  const FunctionDef *def = map_get(s->functions, &key);
  if (!def) {
    // TODO undefined function error
    return NULL;
  }

  bool valid = _check_function_args(s, expr, def);
  if (expr->composable && !tc_check_expr(s, expr->composable)) {
    valid = false; // No need to error here; composable typecheck should report
  }

  return valid ? def->return_type : NULL;
}

static Type *_import_type(Symbols *s, const ImportExpr *expr) {
  switch (expr->type) {
    case IMPORT_URL: return _builtin_type(T_STRING);
    case IMPORT_CSV: {
      Type *string_type = _builtin_type(T_STRING);
      Type *record_type = _map_of(string_type, string_type);

      Type *row_type = or_default(expr->data_type, record_type);
      Type **types = tc_new_array(Type *, 2);
      types[0] = record_type;
      types[1] = _list_of(row_type);
      return _tuple_of(types, 2);
    }
    default: assert(false); // Unreachable
  }
}

static Type *_env_type(Symbols *s, const EnvExpr *expr) {
  if (expr->default_value) {
    if (!_is_builtin_type(tc_check_expr(s, expr->default_value), T_STRING)) {
      // TODO error: env value expression must be a string
      return NULL;
    }
  }
  return _builtin_type(T_STRING);
}

static Type *_block_type(Symbols *s, const BlockExpr *expr) {
  _push_scope(expr->scope);

  _pop_scope();

  return NULL; // TODO
}

static Type *tc_check_expr(Symbols *s, const Expr *expr) {
  switch (expr->type) {
    case EXPR_LITERAL: return _literal_type(s, expr->literal);
    case EXPR_VARIABLE: return _variable_type(s, expr->variable);
    case EXPR_UNARY: return _unary_type(s, expr->unary);
    case EXPR_BINARY: return _binary_type(s, expr->binary);
    case EXPR_ASSIGNMENT: return _assignment_type(s,expr->assignment);
    case EXPR_IF: return _if_expr_type(s, expr->if_expr);
    case EXPR_FOR: return _for_expr_type(s, expr->for_expr);
    case EXPR_BLOCK: return _block_type(s, expr->block);
    case EXPR_FUNCTION_CALL: return _function_call_type(s, expr->function_call);
    case EXPR_IMPORT: return _import_type(s, expr->import);
    case EXPR_ENV: return _env_type(s, expr->env);
    case EXPR_GROUP: return tc_check_expr(s, expr->group->inner_expr);
  }
}

static void _check_decl(Symbols *s, const DeclarationStmt *decl) {
  Type *value_type = tc_check_expr(s, decl->right);

  if (decl->left_count == 1) {
    Type *decl_type = or_default(decl->type, value_type);
    if (!tc_accepts_type(decl_type, value_type)) {
      // TODO error: type mismatch for declaration
      return;
    }

    SymbolKey key = { .name = decl->left[0]->lexeme, .scope = _peek_scope() };
    map_insert(s->symbols, &key, NULL); // TODO
  } else {
    switch (value_type->type) {
      case T_LIST: {
        Type *decl_type = or_default(decl->type, value_type->list->item_type);
        if (!tc_accepts_type(decl_type, value_type->list->item_type)) {
          // TODO error: type mismatch for declaration
          return;
        }

        for (u32 i = 0; i < decl->left_count; i++) {
          SymbolKey key = { .name = decl->left[i]->lexeme, .scope = _peek_scope() };
          map_insert(s->symbols, &key, NULL); // TODO
        }
      }
      case T_TUPLE_SHORT: {
        Type *decl_type = or_default(decl->type, value_type->short_tuple->type);
        if (!tc_accepts_type(decl_type, value_type->short_tuple->type)) {
          // TODO error: type mismatch for declaration
          return;
        }
        if (decl->left_count > value_type->short_tuple->count) {
          // TODO error: component count mismatch
          return;
        }

        for (u32 i = 0; i < decl->left_count; i++) {
          SymbolKey key = { .name = decl->left[i]->lexeme, .scope = _peek_scope() };
          map_insert(s->symbols, &key, NULL); // TODO
        }
      }
      case T_TUPLE: {
        bool valid = true;
        for (u32 i = 0; i < decl->left_count; i++) {
          if (i >= value_type->tuple->len) {
            // TODO error: component count mismatch
            return;
          }

          Type *decl_type = or_default(decl->type, value_type->tuple->types[i]);
          if (!tc_accepts_type(decl_type, value_type->tuple->types[i])) {
            // TODO error: type mismatch for declaration
            return;
          }
        }
        if (!valid) return;

        for (u32 i = 0; i < decl->left_count; i++) {
          SymbolKey key = { .name = decl->left[i]->lexeme, .scope = _peek_scope() };
          map_insert(s->symbols, &key, NULL); // TODO
        }
      }
      default: {
        // TODO error decomposition not allowed for this type
      }
    }
  }
}

static void _check_constant(Symbols *s, const ConstantStmt *constant) {
  Type *value_type = tc_check_expr(s, constant->right);

  Type *decl_type = or_default(constant->type, value_type);
  if (!tc_accepts_type(decl_type, value_type)) {
    // TODO error: type mismatch for declaration
    return;
  }

  SymbolKey key = { .name = constant->left->lexeme, .scope = _peek_scope() };
  map_insert(s->symbols, &key, NULL); // TODO
}

static void _check_function_def(Symbols *s, const FunctionDef *def) {
  _push_scope(def->scope);

  // TODO check param default values
  // TODO define params

  Type *body_type = _block_type(s, def->body);
  if (!tc_accepts_type(def->return_type, body_type)) {
    // TODO function return type mismatch error
  }

  _pop_scope();
}

static void _check_export(Symbols *s, const ExportStmt *export) {
  // TODO
}

static Type *tc_check_stmt(Symbols *s, const Stmt *stmt) {
  switch (stmt->type) {
    case STMT_EXPR: return tc_check_expr(s, stmt->expr);
    case STMT_DECLARATION: _check_decl(s, stmt->decl); return NULL;
    case STMT_CONSTANT: _check_constant(s, stmt->constant); return NULL;
    case STMT_FUNCTION_DEF: _check_function_def(s, stmt->function_def); return NULL;
    case STMT_EXPORT: _check_export(s, stmt->export); return NULL;
    case STMT_TYPE_ALIAS: return NULL;
    case STMT_PARSE_ERROR: assert(false); // Unreachable
  }
}

CompilationStatus sa_build_tables_and_typecheck(i32 file_idx) {
  // TODO
  return STATUS_SUCCEEDED;
}

void sa_free_symbols(Symbols *s) {
  if (!s) return;

  if (s->types) { map_free(s->types); free(s->types); }
  if (s->functions) { map_free(s->functions); free(s->functions); }
  if (s->symbols) { map_free(s->symbols); free(s->symbols); }
  free(s);
}
