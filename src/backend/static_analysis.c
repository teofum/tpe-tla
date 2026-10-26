#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ast/ast.h>
#include <ast/symbols.h>
#include <error/error.h>
#include <frontend/bison/bison_parser.h>
#include <support/dyn_array.h>
#include <support/hash_map.h>
#include <support/logger.h>
#include <support/mem.h>
#include <support/state.h>
#include <support/types.h>
#include <support/util.h>

#include "static_analysis.h"

typedef struct {
  Logger *sym_logger;
  Logger *tc_logger;

  dyn_array *stack;
  u32 next_scope_id;

  dyn_arena *tc_arena;
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

static u32 _get_scope(u32 level) {
  return *(u32 *)dyn_array_get(sa->stack, dyn_array_size(sa->stack) - 1 - level);
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
  sa->tc_arena = mem_dyn_arena_create(MEM_SIZE_AUTO);
}

void sa_shutdown() {
  if (!sa) return;

  if (sa->stack) dyn_array_free(sa->stack);
  if (sa->sym_logger) logger_free(sa->sym_logger);
  if (sa->tc_logger) logger_free(sa->tc_logger);
  if (sa->tc_arena) mem_dyn_arena_destroy(sa->tc_arena);
  free(sa);

  sa = NULL;
  cs = NULL;
}

static void tc_debug_log(const char *s, const Location *loc) {
  logger_log(sa->tc_logger, LOG_DEBUG, "%s @ %u:%u - %u:%u", s,
    loc->first_line, loc->first_column,
    loc->last_line, loc->last_column
  );
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
    case L_VECTOR: {
      _static_tables_expr(s, literal->vec->value);
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

#define tc_new(T) (((T*)mem_dyn_arena_calloc(sa->tc_arena, 1, sizeof(T))))
#define tc_new_array(T, n) ((T*)(mem_dyn_arena_calloc(sa->tc_arena, n, sizeof(T))))

// C is for Cursed macro dark arts
#define tc_make(T, name, ...)               \
  T *name = tc_new(T);                      \
  *name = (T)__VA_ARGS__

static Identifier *tc_clone_id(const Identifier *id) {
  if (!id) return NULL;

  Identifier *clone = tc_new(Identifier);
  *clone = (Identifier){
    .label = id->label,
    .len = id->len,
    .lexeme = tc_new_array(char, id->len + 1),
    .loc = id->loc,
  };
  strncpy(clone->lexeme, id->lexeme, id->len);

  return clone;
}

static Type *_builtin_type(BuiltinType builtin) {
  tc_make(Type, type, { .type = T_BUILTIN, .builtin = builtin, .meta = {0} });
  return type;
}

static Type _error_type = { .type = T_ERROR };

// Returns an error type.
static Type *tc_error() { return &_error_type; };

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

static Type *_vector_of(Type *item_type, u32 count) {
  tc_make(VectorType, vec, {
    .item_type = item_type,
    .len = count,
    .meta = {0},
  });
  tc_make(Type, type, { .type = T_VECTOR, .vec = vec, .meta = {0} });
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

static Type *_union_of(u32 n, ...) {
  tc_make(UnionType, union_type, {
    .len = n,
    .types = tc_new_array(Type *, n),
    .meta = {0},
  });

  va_list args;
  va_start(args, n);
  for (u32 i = 0; i < n; i++) union_type->types[i] = va_arg(args, Type *);
  va_end(args);

  tc_make(Type, type, {
    .type = T_UNION,
    .union_type = union_type,
    .meta = {0}
  });
  return type;
}

static const char *_builtin_type_names[] = {
  [T_VOID]    = "void",
  [T_NIL]     = "nil",
  [T_INT]     = "int",
  [T_FLOAT]   = "float",
  [T_STRING]  = "string",
  [T_BOOL]    = "bool",
  [T_ELEMENT] = "element",
};

static const char *tc_print_type(const Type *type);

static const char *_tc_asprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  usize len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  va_start(args, fmt);
  char *string = mem_dyn_arena_malloc(sa->tc_arena, len + 1);
  vsnprintf(string, len + 1, fmt, args);
  va_end(args);

  return string;
}

static const char *_print_type_list(
  Type **types,
  u32 len,
  const char *start,
  const char *end
) {
  const char *string = _tc_asprintf("%s%s", start, tc_print_type(types[0]));
  for (u32 i = 1; i < len; i++) {
    string = _tc_asprintf("%s, %s", string, tc_print_type(types[i]));
  }
  string = _tc_asprintf("%s%s", string, end);
  return string;
}

static const char *_print_field(const StructField *field) {
  return _tc_asprintf("%s: %s", field->name, tc_print_type(field->type));
}

static const char *tc_print_type(const Type *type) {
  switch (type->type) {
    case T_BUILTIN: return _builtin_type_names[type->builtin];
    case T_NAMED:   return type->named->name->lexeme;
    case T_ERROR:   return "[ERROR TYPE]";
    case T_RANGE:   return "range";
    case T_LIST: {
      return _tc_asprintf("%s[]", tc_print_type(type->list->item_type));
    }
    case T_VECTOR: {
      return _tc_asprintf(
        "[%s; %u]",
        tc_print_type(type->vec->item_type), type->vec->len
      );
    }
    case T_MAP: {
      return _tc_asprintf(
        "%s[%s]",
        tc_print_type(type->map->value_type), tc_print_type(type->map->key_type)
      );
    }
    case T_TUPLE: {
      return _print_type_list(type->tuple->types, type->tuple->len, "[", "]");
    }
    case T_UNION: {
      return _print_type_list(
        type->union_type->types, type->union_type->len,
        "union { ", " }"
      );
    }
    case T_STRUCT: {
      StructField **fields = type->struct_type->fields;
      const char *string = _tc_asprintf("struct { %s", _print_field(fields[0]));
      for (u32 i = 1; i < type->struct_type->len; i++) {
        string = _tc_asprintf("%s, %s", string, _print_field(fields[i]));
      }
      return _tc_asprintf("%s }", string);
    }
    case T_ENUM: {
      Identifier **variants = type->enum_type->values;
      const char *string = _tc_asprintf("enum { %s", variants[0]->lexeme);
      for (u32 i = 1; i < type->enum_type->len; i++) {
        string = _tc_asprintf("%s, %s", string, variants[i]->lexeme);
      }
      return _tc_asprintf("%s }", string);
    }
  }
}

/* -- Type checking: variables ---------------------------------------------- */

static const Type *tc_resolve_named_type(Symbols *s, const NamedType *named) {
  const TypeAliasStmt *alias = NULL;
  u32 level = 0, scope = 0;

  do {
    scope = _get_scope(level++);
    SymbolKey key = { .name = named->name->lexeme, .scope = scope };
    alias = map_get(s->types, &key);
  } while (!alias && scope != 0);

  if (!alias) {
    logger_log(sa->tc_logger, LOG_ERROR,
      "Scope %u: Type %s is undefined",
      _peek_scope(), named->name->lexeme
    );

    UndefError *error = new(UndefError);
    *error = (UndefError){
      .object_type = "type",
      .object_name = strdup(named->name->lexeme),
    };
    err_report_undef(error, &named->meta.loc, ERR_ERROR);
    return NULL;
  }

  return alias->type;
}

static const FunctionDef *tc_resolve_function(Symbols *s, const Identifier *name) {
  const FunctionDef *function = NULL;
  u32 level = 0, scope = 0;

  do {
    scope = _get_scope(level++);
    SymbolKey key = { .name = name->lexeme, .scope = scope };
    function = map_get(s->functions, &key);
  } while (!function && scope != 0);

  if (!function) {
    logger_log(sa->tc_logger, LOG_ERROR,
      "Scope %u: Function %s is undefined",
      _peek_scope(), name->lexeme
    );

    UndefError *error = new(UndefError);
    *error = (UndefError){
      .object_type = "function",
      .object_name = strdup(name->lexeme),
    };
    err_report_undef(error, &name->loc, ERR_ERROR);
    return NULL;
  }

  return function;
}

static const Symbol *tc_resolve_symbol(Symbols *s, const Identifier *name) {
  Symbol *symbol = NULL;
  u32 level = 0, scope = 0;

  do {
    scope = _get_scope(level++);
    SymbolKey key = { .name = name->lexeme, .scope = scope };
    symbol = map_get(s->symbols, &key);
  } while (!symbol && scope != 0);

  if (!symbol) {
    logger_log(sa->tc_logger, LOG_ERROR,
      "Scope %u: Variable %s is undefined",
      _peek_scope(), name->lexeme
    );

    UndefError *error = new(UndefError);
    *error = (UndefError){
      .object_type = "variable",
      .object_name = strdup(name->lexeme),
    };
    err_report_undef(error, &name->loc, ERR_ERROR);
    return NULL;
  }

  return symbol;
}

static bool tc_define_symbol(Symbols *s, const Identifier *name, Symbol *symbol) {
  SymbolKey key = { .name = name->lexeme, .scope = _peek_scope() };
  Symbol *existing = map_get(s->symbols, &key);

  if (existing) {
    logger_log(sa->sym_logger, LOG_ERROR,
      "Scope %u: Redefinition of variable %s",
      key.scope, key.name
    );

    RedefError *error = new(RedefError);
    *error = (RedefError){
      .object_type = "variable",
      .object_name = strdup(name->lexeme),
      .previous_definition = existing->loc
    };
    err_report_redef(error, &name->loc, ERR_ERROR);
    return false;
  }

  symbol->loc = name->loc;
  map_insert(s->symbols, &key, symbol);
  return true;
}

static Symbol *_symbol_of(Type *type, bool constant) {
  tc_make(Symbol, symbol, { .type = type, .constant = constant });
  return symbol;
}

/* -- Type checking: compare ------------------------------------------------ */

static bool tc_compare_types(Symbols *s, const Type *a, const Type *b);
static bool tc_accepts_type(Symbols *s, const Type *a, const Type *b);

static bool _is_builtin(const Type *type, BuiltinType builtin) {
  return type->type == T_BUILTIN && type->builtin == builtin;
}

static bool _is_error(const Type *type) {
  return type->type == T_ERROR;
}

static bool _is_numeric_type(const Type *type) {
  return type->type == T_BUILTIN
    && (type->builtin == T_INT || type->builtin == T_FLOAT);
}

static bool _is_numvec_type(const Type *type) {
  if (type->type == T_VECTOR && _is_numeric_type(type->vec->item_type)) {
    return true;
  } else if (type->type == T_TUPLE) {
    for (u32 i = 0; i < type->tuple->len; i++) {
      if (!_is_numeric_type(type->tuple->types[i])) return false;
    }
    return true;
  } else {
    return false;
  }
}

// Assumes type is a numeric vector!
static BuiltinType _numvec_item_type(const Type *type) {
  switch (type->type) {
    case T_VECTOR: return type->vec->item_type->builtin;
    case T_TUPLE: return T_FLOAT;
    default: assert(false); // Unreachable
  }
}

// Assumes type is a numeric vector!
static u32 _numvec_len(const Type *type) {
  switch (type->type) {
    case T_VECTOR: return type->vec->len;
    case T_TUPLE: return type->tuple->len;
    default: assert(false); // Unreachable
  }
}

static bool _is_iterable(const Type *type) {
  return type->type == T_VECTOR || type->type == T_LIST;
}

static Type *_upgrade_numeric_type(BuiltinType l, BuiltinType r) {
  if (l == T_INT && r == T_INT) return _builtin_type(T_INT);
  return _builtin_type(T_FLOAT);
}

static bool _compare_list_types(Symbols *s, const ListType *a, const ListType *b) {
  return tc_compare_types(s, a->item_type, b->item_type);
}

static bool _compare_vector_types(Symbols *s, const VectorType *a, const VectorType *b) {
  return a->len == b->len
    && tc_compare_types(s, a->item_type, b->item_type);
}

static bool _compare_tuple_types(Symbols *s, const TupleType *a, const TupleType *b) {
  if (a->len != b->len) return false;
  for (u32 i = 0; i < a->len; i++) {
    if (!tc_compare_types(s, a->types[i], b->types[i])) return false;
  }
  return true;
}

static bool _compare_map_types(Symbols *s, const MapType *a, const MapType *b) {
  return tc_compare_types(s, a->key_type, b->key_type)
    && tc_compare_types(s, a->value_type, b->value_type);
}

static bool _compare_struct_types(Symbols *s, const StructType *a, const StructType *b) {
  if (a->len != b->len) return false;
  for (u32 i = 0; i < a->len; i++) {
    StructField *fa = a->fields[i];
    StructField *fb = b->fields[i];
    if (!ast_compare_identifiers(fa->name, fb->name)) return false;
    if (!tc_compare_types(s, fa->type, fb->type)) return false;
  }
  return true;
}

static bool _compare_union_types(Symbols *s, const UnionType *a, const UnionType *b) {
  if (a->len != b->len) return false;
  for (u32 i = 0; i < a->len; i++) {
    if (!tc_compare_types(s, a->types[i], b->types[i])) return false;
  }
  return true;
}

static bool _compare_enum_types(const EnumType *a, const EnumType *b) {
  if (a->len != b->len) return false;
  for (u32 i = 0; i < a->len; i++) {
    if (!ast_compare_identifiers(a->values[i], b->values[i])) return false;
  }
  return true;
}

static bool _compare_named_types(Symbols *s, const NamedType *a, const NamedType *b) {
  if (ast_compare_identifiers(a->name, b->name)) return true;
  return tc_compare_types(
    s,
    tc_resolve_named_type(s, a),
    tc_resolve_named_type(s, b)
  );
}

static bool tc_compare_types(Symbols *s, const Type *a, const Type *b) {
  if (a->type == T_ERROR || b->type == T_ERROR) return false;

  // If the types are literally the same object they are obviously equal
  if (a == b) return true;

  if (a->type == T_NAMED) {
    return tc_compare_types(s, tc_resolve_named_type(s, a->named), b);
  }
  if (b->type == T_NAMED) {
    return tc_compare_types(s, a, tc_resolve_named_type(s, b->named));
  }

  if (a->type != b->type) return false;
  switch (a->type) {
    case T_BUILTIN: return a->builtin == b->builtin;
    case T_LIST: return _compare_list_types(s, a->list, b->list);
    case T_VECTOR: return _compare_vector_types(s, a->vec, b->vec);
    case T_TUPLE: return _compare_tuple_types(s, a->tuple, b->tuple);
    case T_MAP: return _compare_map_types(s, a->map, b->map);
    case T_STRUCT: return _compare_struct_types(s, a->struct_type, b->struct_type);
    case T_UNION: return _compare_union_types(s, a->union_type, b->union_type);
    case T_ENUM: return _compare_enum_types(a->enum_type, b->enum_type);
    case T_RANGE: return true; // Ranges are only one type
    case T_NAMED: assert(false);
    case T_ERROR: assert(false);
  }
}

static bool _accepts_builtin_type(BuiltinType a, const Type *b) {
  return b->type == T_BUILTIN
    && (b->builtin == a || (a == T_FLOAT && b->builtin == T_INT));
}

static bool _accepts_list_type(Symbols *s, const ListType *a, const Type *b) {
  switch (b->type) {
    case T_LIST: {
      return tc_accepts_type(s, a->item_type, b->list->item_type);
    }
    case T_VECTOR: {
      return tc_accepts_type(s, a->item_type, b->vec->item_type);
    }
    case T_TUPLE: {
      for (u32 i = 0; i < b->tuple->len; i++) {
        if (!tc_accepts_type(s, a->item_type, b->tuple->types[i])) return false;
      }
      return true;
    }
    default: return false;
  }
}

static bool _accepts_vector_type(Symbols *s, const VectorType *a, const Type *b) {
  switch (b->type) {
    case T_VECTOR: {
      return a->len == b->vec->len
        && tc_accepts_type(s, a->item_type, b->vec->item_type);
    }
    case T_TUPLE: {
      if (a->len != b->tuple->len) return false;
      for (u32 i = 0; i < b->tuple->len; i++) {
        if (!tc_accepts_type(s, a->item_type, b->tuple->types[i])) return false;
      }
      return true;
    }
    default: return false;
  }
}

static bool _accepts_tuple_type(Symbols *s, const TupleType *a, const Type *b) {
  switch (b->type) {
    case T_VECTOR: {
      if (a->len != b->vec->len) return false;
      for (u32 i = 0; i < a->len; i++) {
        if (!tc_accepts_type(s, a->types[i], b->vec->item_type)) return false;
      }
      return true;
    }
    case T_TUPLE: {
      if (a->len != b->tuple->len) return false;
      for (u32 i = 0; i < b->tuple->len; i++) {
        if (!tc_accepts_type(s, a->types[i], b->tuple->types[i])) return false;
      }
      return true;
    }
    default: return false;
  }
}

static bool _accepts_map_type(Symbols *s, const MapType *a, const Type *b) {
  if (b->type != T_MAP) return false;
  return tc_compare_types(s, a->key_type, b->map->key_type)
    && tc_accepts_type(s, a->value_type, b->map->value_type);
}

static bool _accepts_struct_type(Symbols *s, const StructType *a, const Type *b) {
  if (b->type != T_STRUCT) return false;
  return _compare_struct_types(s, a, b->struct_type); // TODO
}

static bool _accepts_union_type(Symbols *s, const UnionType *a, const Type *b) {
  if (b->type == T_UNION) {
    for (u32 i = 0; i < b->union_type->len; i++) {
      if (!_accepts_union_type(s, a, b->union_type->types[i])) return false;
    }
    return true;
  } else {
    for (u32 i = 0; i < a->len; i++) {
      if (tc_accepts_type(s, a->types[i], b)) return true;
    }
    return false;
  }
}

static bool tc_accepts_type(Symbols *s, const Type *a, const Type *b) {
  if (a->type == T_ERROR || b->type == T_ERROR) return false;

  // If the types are literally the same object they are obviously equal
  if (b == a) return true;

  if (a->type == T_NAMED) {
    return tc_accepts_type(s, tc_resolve_named_type(s, a->named), b);
  }
  if (b->type == T_NAMED) {
    return tc_accepts_type(s, a, tc_resolve_named_type(s, b->named));
  }

  switch (a->type) {
    case T_BUILTIN: return _accepts_builtin_type(a->builtin, b);
    case T_LIST: return _accepts_list_type(s, a->list, b);
    case T_VECTOR: return _accepts_vector_type(s, a->vec, b);
    case T_TUPLE: return _accepts_tuple_type(s, a->tuple, b);
    case T_MAP: return _accepts_map_type(s, a->map, b);
    case T_STRUCT: return _accepts_struct_type(s, a->struct_type, b);
    case T_UNION: return _accepts_union_type(s, a->union_type, b);
    case T_ENUM: return tc_compare_types(s, a, b);
    case T_RANGE: return b->type == T_RANGE;
    case T_NAMED: assert(false);
    case T_ERROR: assert(false);
  }
}

static bool tc_compatible_types(Symbols *s, const Type *a, const Type *b) {
  return tc_accepts_type(s, a, b) || tc_accepts_type(s, b, a);
}

/* -- Type checking: errors ------------------------------------------------- */

static Type *tc_report_error(const Location *loc, const char *message) {
  logger_log(sa->tc_logger, LOG_ERROR, "Type Error: %s", message);

  TypeError *error = new(TypeError);
  *error = (TypeError){ .message = strdup(message) };
  err_report_type(error, loc, ERR_ERROR);

  return tc_error(); // Return the error type for convenience
}

static Type *tc_report_error_fmt(const Location *loc, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char *message;
  vasprintf(&message, fmt, args);
  va_end(args);

  logger_log(sa->tc_logger, LOG_ERROR, "Type Error: %s", message);

  TypeError *error = new(TypeError);
  *error = (TypeError){ .message = message };
  err_report_type(error, loc, ERR_ERROR);

  return tc_error(); // Return the error type for convenience
}

static Type *tc_report_bad_index_type(const Location *loc, const Type *type) {
  // Fail silently on error type; assume error was already reported upstream
  if (_is_error(type)) return tc_error();

  return tc_report_error_fmt(
    loc,
    "expression of type %s is not a valid index type", tc_print_type(type)
  );
}

static Type *tc_report_op_type(
  const Location *loc,
  const char *expected,
  const Type *got
) {
  // Fail silently on error type; assume error was already reported upstream
  if (_is_error(got)) return tc_error();

  return tc_report_error_fmt(
    loc,
    "invalid operand type: expected %s, got %s",
    expected, tc_print_type(got)
  );
}

/* -- Type checking: check -------------------------------------------------- */

static Type *tc_check_expr(Symbols *s, const Expr *expr);
static Type *tc_check_stmt(Symbols *s, const Stmt *stmt);
static Type *tc_check_block(Symbols *s, const BlockExpr *expr);

static Type *_list_type(Symbols *s, ListLiteral *list) {
  tc_debug_log("List", &list->meta.loc);

  Type **types = tc_new_array(Type *, list->len);
  bool homogenous = true;
  for (u32 i = 0; i < list->len; i++) {
    types[i] = tc_check_expr(s, list->exprs[i]);
    if (i > 0 && !tc_compare_types(s, types[0], types[i])) homogenous = false;
  }

  return homogenous
    ? _vector_of(types[0], list->len)
    : _tuple_of(types, list->len);
}

static Type *_vector_type(Symbols *s, VectorLiteral *vec) {
  Type *item_type = tc_check_expr(s, vec->value);
  if (_is_error(item_type)) return tc_error();

  return _vector_of(item_type, vec->len);
}

static bool _is_valid_map_key_type(const Type *type) {
  switch (type->type) {
    case T_BUILTIN:       return type->builtin != T_NIL && type->builtin != T_VOID;
    case T_NAMED:         assert(false); // Named types should not make it this far!
    case T_ERROR:         return false;
    case T_UNION:         return false;
    case T_MAP:           return false;
    case T_LIST:          return false;
    case T_RANGE:         return false;
    case T_ENUM:          return true;
    case T_VECTOR:   return _is_valid_map_key_type(type->vec->item_type);
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

static Type *_add_to_union_type(Symbols *s, Type *value_type, Type *new_type) {
  if (_is_error(value_type) || _is_error(new_type)) return tc_error();

  if (value_type->type == T_UNION) {
    if (_accepts_union_type(s, value_type->union_type, new_type)) {
      return value_type;
    }

    UnionType *u = value_type->union_type;
    u->len++;
    u->types = realloc(u->types, u->len * sizeof(Type *)); // FIXME leaks and stinks!!
    return value_type;
  } else {
    if (tc_accepts_type(s, value_type, new_type)) {
      return value_type;
    }

    return _union_of(2, value_type, new_type);
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

    if (!tc_compare_types(s, key_type, k)) {
      TypeError *error = new(TypeError);
      *error = (TypeError){ .message = "inconsistent key type" };
      err_report_type(error, &map->entries[i]->key->meta.loc, ERR_ERROR);
      valid = false;
    }

    value_type = _add_to_union_type(s, value_type, v);
  }

  if (_is_error(value_type)) return tc_error();
  return valid ? _map_of(key_type, value_type) : tc_error();
}

static Type *_struct_type(Symbols *s, StructLiteral *struct_l) {
  StructField **fields = tc_new_array(StructField *, struct_l->len);
  bool valid = true;

  for (u32 i = 0; i < struct_l->len; i++) {
    StructLiteralField *lf = struct_l->fields[i];
    fields[i] = tc_new(StructField);
    *fields[i] = (StructField) {
      .name = tc_clone_id(lf->name),
      .type = tc_check_expr(s, lf->value),
    };
    if (_is_error(fields[i]->type)) valid = false;
  }

  if (!valid) return tc_error();

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
  bool start_valid = !range->start
    || _is_builtin(tc_check_expr(s, range->start), T_INT);
  bool end_valid = !range->end
    || _is_builtin(tc_check_expr(s, range->end), T_INT);

  if (!start_valid || !end_valid) {
    return tc_report_error(
      &range->meta.loc,
      "range start/end expressions must have integer type"
    );
  }

  tc_make(Type, type, { .type = T_RANGE, .meta = {0} });
  return type;
}

static Type *_literal_type(Symbols *s, LiteralExpr *expr) {
  tc_debug_log("Literal", &expr->meta.loc);

  switch (expr->type) {
    case L_INTEGER:     return _builtin_type(T_INT);
    case L_FLOAT:       return _builtin_type(T_FLOAT);
    case L_STRING:      return _builtin_type(T_STRING);
    case L_BOOL:        return _builtin_type(T_BOOL);
    case L_NIL:         return _builtin_type(T_NIL);
    case L_LIST:        return _list_type(s, expr->list);
    case L_VECTOR:      return _vector_type(s, expr->vec);
    case L_MAP:         return _map_type(s, expr->map);
    case L_STRUCT:      return _struct_type(s, expr->struct_literal);
    case L_ENUM:        return _enum_type(expr->enum_literal);
    case L_RANGE:       return _range_type(s, expr->range);
  }
}

static Type *_struct_member_type(const StructType *s, const Identifier *field) {
  for (u32 i = 0; i < s->len; i++) {
    if (strcmp(s->fields[i]->name->lexeme, field->lexeme) == 0) {
      return s->fields[i]->type;
    }
  }

  return tc_report_error_fmt(
    &field->loc,
    "struct has no member '%s'", field->lexeme
  );
}

static Type *_indexed_list_item_type(Symbols *s, Type *item_type, Expr *index) {
  Type *index_type = tc_check_expr(s, index);

  if (index_type->type == T_RANGE) return _list_of(item_type);
  if (_is_builtin(index_type, T_INT)) return item_type;

  return tc_report_bad_index_type(&index->meta.loc, index_type);
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

    return valid ? _tuple_of(types, var->index_count) : tc_error();
  }
}

static Type *_indexed_tuple_item_type(Symbols *s, TupleType *tuple, Expr *index) {
  Type *index_type = tc_check_expr(s, index);
  if (!_is_builtin(index_type, T_INT)) {
    return tc_report_bad_index_type(&index->meta.loc, index_type);
  }

  // If the index is an integer literal, we can infer the actual item type since
  // the value is known at time of type checking
  if (index->type == EXPR_LITERAL && index->literal->type == L_INTEGER) {
    i64 idx = index->literal->integer->value;
    if (idx < 0) idx += tuple->len;
    if (idx < 0 || idx >= tuple->len) {
      return tc_report_error_fmt(
        &index->meta.loc,
        "index %ld is out of bounds for tuple of size %u", idx, tuple->len
      );
    }
    return tuple->types[idx];
  }

  // For any other expression we'd need to evaluate it to know which tuple item
  // it references, so we just return a union of all types in the tuple
  Type *value_type = tuple->types[0];
  for (u32 i = 1; i < tuple->len; i++) {
    value_type = _add_to_union_type(s, value_type, tuple->types[i]);
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

    return valid ? _tuple_of(types, var->index_count) : tc_error();
  }
}

static Type *_indexed_vector_type(Symbols *s, VectorType *vec, IndexedVariable *var) {
  if (var->index_count == 1) {
    Type *index_type = tc_check_expr(s, var->indices[0]);
    if (!_is_builtin(index_type, T_INT)) {
      return tc_report_bad_index_type(&var->indices[0]->meta.loc, index_type);
    }
    return vec->item_type;
  } else {
    bool valid = true;
    for (u32 i = 0; i < var->index_count; i++) {
      Type *index_type = tc_check_expr(s, var->indices[i]);
      if (!_is_builtin(index_type, T_INT)) {
        return tc_report_bad_index_type(&var->indices[i]->meta.loc, index_type);
      }
    }

    return valid ? _vector_of(vec->item_type, var->index_count) : tc_error();
  }
}

static Type *_indexed_map_type(Symbols *s, MapType *map, IndexedVariable *var) {
  if (var->index_count == 1) {
    Type *index_type = tc_check_expr(s, var->indices[0]);
    if (!tc_accepts_type(s, map->key_type, index_type)) {
      return tc_report_error_fmt(
        &var->indices[0]->meta.loc,
        "invalid key type: expected %s, got %s",
        tc_print_type(map->key_type), tc_print_type(index_type)
      );
    }
    return map->value_type;
  } else {
    bool valid = true;
    for (u32 i = 0; i < var->index_count; i++) {
      Type *index_type = tc_check_expr(s, var->indices[i]);
      if (!tc_accepts_type(s, map->key_type, index_type)) {
        return tc_report_error_fmt(
          &var->indices[i]->meta.loc,
          "invalid key type: expected %s, got %s",
          tc_print_type(map->key_type), tc_print_type(index_type)
        );
      }
    }

    return valid ? _vector_of(map->value_type, var->index_count) : tc_error();
  }
}

static Type *_variable_type(Symbols *s, VariableExpr *var) {
  tc_debug_log("Var", &var->meta.loc);

  switch (var->type) {
    case V_NAMED: {
      const Symbol *symbol = tc_resolve_symbol(s, var->named->name);
      return symbol ? symbol->type : tc_error();
    }
    case V_INDEX: {
      Type *container = _variable_type(s, var->indexed->container);
      switch (container->type) {
        case T_LIST: return _indexed_list_type(s, container->list, var->indexed);
        case T_TUPLE: return _indexed_tuple_type(s, container->tuple, var->indexed);
        case T_VECTOR: return _indexed_vector_type(s, container->vec, var->indexed);
        case T_MAP: return _indexed_map_type(s, container->map, var->indexed);
        default: {
          return tc_report_error_fmt(
            &var->indexed->meta.loc,
            "type %s is not indexable", tc_print_type(container)
          );
        }
      }
    }
    case V_STRUCT_MEMBER: {
      Type *container = _variable_type(s, var->struct_member->struct_expr);
      if (container->type != T_STRUCT) {
        return tc_report_error_fmt(
          &var->struct_member->meta.loc,
          "type %s is not a struct", tc_print_type(container)
        );
      }
      return _struct_member_type(container->struct_type, var->struct_member->name);
    }
  }
}

static Type *_unary_type(Symbols *s, UnaryExpr *expr) {
  tc_debug_log("UnaryExpr", &expr->meta.loc);

  Type *expr_type = tc_check_expr(s, expr->expr);
  switch (expr->op) {
    case BANG:
    case NOT: {
      if (!_is_builtin(expr_type, T_BOOL)) {
        return tc_report_op_type(
          &expr->expr->meta.loc,
          "boolean", expr_type
        );
      }
      return expr_type;
    }
    case MINUS: {
      if (!_is_builtin(expr_type, T_INT) && !_is_builtin(expr_type, T_FLOAT)) {
        return tc_report_op_type(
          &expr->expr->meta.loc,
          "numeric type", expr_type
        );
      }
      return expr_type;
    }
    default: assert(false); // Unreachable
  }
}

static Type *_arithmetic_numeric_type(Symbols *s, Type *l, Type *r, BinaryExpr *expr) {
  if (_is_numeric_type(r)) {
    return _upgrade_numeric_type(l->builtin, r->builtin);
  } else if (_is_numvec_type(r)) {
    return _vector_of(
      _upgrade_numeric_type(l->builtin, _numvec_item_type(r)),
      _numvec_len(r)
    );
  } else {
    return tc_report_op_type(
      &expr->right->meta.loc,
      "numeric or vector type", r
    );
  }
}

static Type *_arithmetic_vector_type(Symbols *s, Type *l, Type *r, BinaryExpr *expr) {
  if (_is_numvec_type(r)) {
    if (_numvec_len(l) != _numvec_len(r)) {
      return tc_report_error(&expr->meta.loc, "vector length mismatch");
    }
    Type *num_t = _upgrade_numeric_type(
      _numvec_item_type(l),
      _numvec_item_type(r)
    );
    return _vector_of(num_t, _numvec_len(l));
  } else if (_is_numeric_type(r)) {
    return _vector_of(
      _upgrade_numeric_type(_numvec_item_type(l), r->builtin),
      _numvec_len(l)
    );
  } else {
    return tc_report_op_type(
      &expr->right->meta.loc,
      "numeric or vector type", r
    );
  }
}

static Type *_binary_type(Symbols *s, BinaryExpr *expr) {
  tc_debug_log("BinaryExpr", &expr->meta.loc);

  Type *l_type = tc_check_expr(s, expr->left);
  Type *r_type = tc_check_expr(s, expr->right);
  switch (expr->op) {
    case PLUS:
    case STAR: {
      if (_is_numeric_type(l_type)) {
        return _arithmetic_numeric_type(s, l_type, r_type, expr);
      } else if (_is_numvec_type(l_type)) {
        return _arithmetic_vector_type(s, l_type, r_type, expr);
      } else if (_is_builtin(l_type, T_STRING)) {
        if (expr->op == PLUS) {
          if (!_is_error(r_type)) {
            return l_type;
          } else {
            return tc_error();
          }
        } else if (expr->op == STAR) {
          if (_is_builtin(r_type, T_INT)) {
            return l_type;
          } else {
            return tc_report_op_type(&expr->right->meta.loc, "integer", r_type);
          }
        }
      } else {
        return tc_report_op_type(
          &expr->left->meta.loc,
          "numeric or vector type, string", l_type
        );
      }
    }
    case MINUS:
    case SLASH: {
      if (_is_numeric_type(l_type)) {
        return _arithmetic_numeric_type(s, l_type, r_type, expr);
      } else if (_is_numvec_type(l_type)) {
        return _arithmetic_vector_type(s, l_type, r_type, expr);
      } else {
        return tc_report_op_type(
          &expr->left->meta.loc,
          "numeric or vector type", l_type
        );
      }
    }
    case GREATER_EQUAL:
    case GREATER:
    case LESS_EQUAL:
    case LESS: {
      if (_is_numeric_type(l_type)) {
        if (_is_numeric_type(r_type)) {
          return _builtin_type(T_BOOL);
        } else {
          return tc_report_op_type(
            &expr->right->meta.loc,
            "numeric type", r_type
          );
        }
      } else if (_is_builtin(l_type, T_STRING)) {
        if (_is_builtin(r_type, T_STRING)) {
          return _builtin_type(T_BOOL);
        } else {
          return tc_report_op_type(&expr->right->meta.loc, "string", r_type);
        }
      } else {
        return tc_report_op_type(
          &expr->left->meta.loc,
          "numeric type, string", l_type
        );
      }
    }
    case EQUAL_EQUAL:
    case BANG_EQUAL: {
      if (!tc_compatible_types(s, l_type, r_type)) {
        return tc_report_error_fmt(
          &expr->meta.loc,
          "%s and %s are not comparable",
          tc_print_type(l_type), tc_print_type(r_type)
        );
      }
      return _builtin_type(T_BOOL);
    }
    case AND:
    case OR: {
      bool left_valid = _is_builtin(l_type, T_BOOL);
      bool right_valid = _is_builtin(r_type, T_BOOL);
      if (!left_valid) {
        return tc_report_op_type(&expr->left->meta.loc, "boolean", l_type);
      }
      if (!right_valid) {
        return tc_report_op_type(&expr->right->meta.loc, "boolean", r_type);
      }
      return (left_valid && right_valid) ? l_type : tc_error();
    }
    default: assert(false); // Unreachable
  }
}

static Type *_assignment_type(Symbols *s, const AssignmentExpr *expr) {
  tc_debug_log("Assignment", &expr->meta.loc);

  Type *lhs_type = _variable_type(s, expr->left);
  Type *rhs_type = tc_check_expr(s, expr->right);

  // Fail silently on error type; assume error has been reported upstream
  if (_is_error(lhs_type) || _is_error(rhs_type)) return tc_error();

  if (tc_accepts_type(s, lhs_type, rhs_type)) return lhs_type;
  return tc_report_error_fmt(
    &expr->meta.loc,
    "%s is not assignable to %s",
    tc_print_type(rhs_type), tc_print_type(lhs_type)
  );
}

static Type *_if_expr_type(Symbols *s, const IfExpr *expr) {
  tc_debug_log("IfExpr", &expr->meta.loc);

  Type *condition_type = tc_check_expr(s, expr->condition);
  Type *true_type = tc_check_expr(s, expr->true_branch);
  Type *false_type = expr->false_branch
    ? tc_check_expr(s, expr->false_branch)
    : _builtin_type(T_VOID);

  if (!_is_builtin(condition_type, T_BOOL)) {
    return tc_report_error_fmt(
      &expr->condition->meta.loc,
      "invalid type for if expression condition: expected boolean, got %s",
      tc_print_type(condition_type)
    );
  }

  if (tc_accepts_type(s, true_type, false_type)) {
    return true_type;
  } else if (tc_accepts_type(s, false_type, true_type)) {
    return false_type;
  } else {
    return _union_of(2, true_type, false_type);
  }
}

static Type *_for_expr_type(Symbols *s, const ForExpr *expr) {
  tc_debug_log("ForExpr", &expr->meta.loc);

  Type *iterable_type = tc_check_expr(s, expr->iterable);
  if (!_is_iterable(iterable_type)) {
    return tc_report_error_fmt(
      &expr->iterable->meta.loc,
      "%s is not iterable", tc_print_type(iterable_type)
    );
  }

  Type *iterator_type = iterable_type->type == T_LIST
    ? iterable_type->list->item_type
    : iterable_type->vec->item_type;

  _push_scope(expr->body->block->scope);
  Symbol *iterator = _symbol_of(iterator_type, true);
  tc_define_symbol(s, expr->var_id, iterator);
  if (expr->idx_id) {
    Symbol *idx_iterator = _symbol_of(_builtin_type(T_INT), true);
    tc_define_symbol(s, expr->idx_id, idx_iterator);
  }
  Type *body_type = tc_check_expr(s, expr->body);
  _pop_scope();

  switch (iterable_type->type) {
    case T_LIST: {
      return _list_of(body_type);
    }
    case T_VECTOR: {
      return _vector_of(body_type, iterable_type->vec->len);
    }
    default: assert(false); // Unreachable
  }
}

static u32 _find_function_param(const FunctionDef *def, const Identifier *name) {
  for (u32 j = 0; j < def->params_len; j++) {
    if (ast_compare_identifiers(def->params[j]->name, name)) return j;
  }
  return -1;
}

static bool _check_function_args(
  Symbols *s,
  const FunctionCallExpr *expr,
  const FunctionDef *def
) {
  if (expr->args_len > def->params_len) {
    FnArgCountError *error = new(FnArgCountError);
    *error = (FnArgCountError){
      .expected = def->params_len,
      .actual = expr->args_len,
    };
    err_report_fn_arg_count(error, &expr->meta.loc, ERR_ERROR);
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
        err_report_fn_arg_name(expr, arg, ERR_ERROR);
        valid = false; continue;
      }
    } else {
      if (!positional_args_allowed) {
        err_report_fn_arg_pos(arg, ERR_ERROR);
        valid = false; continue;
      }
      param_idx = i;
    }
    params_set[param_idx] = true;

    Parameter *param = def->params[param_idx];
    Type *arg_type = tc_check_expr(s, arg->value);
    if (!tc_accepts_type(s, param->type, arg_type)) {
      tc_report_error_fmt(
        &param->meta.loc,
        "%s is not assignable to %s",
        tc_print_type(arg_type), tc_print_type(param->type)
      );
      valid = false;
    }
  }

  // Ensure all the required parameters are actually being passed
  for (u32 i = 0; i < def->params_len; i++) {
    if (!params_set[i] && !def->params[i]->default_value) {
      err_report_fn_param_req(expr, def->params[i], ERR_ERROR);
      valid = false;
    }
  }

  return valid;
}

static Type *_function_call_type(Symbols *s, const FunctionCallExpr *expr) {
  tc_debug_log("FunctionCall", &expr->meta.loc);

  const FunctionDef *def = tc_resolve_function(s, expr->name);
  if (!def) return tc_error();

  bool valid = _check_function_args(s, expr, def);
  if (expr->composable && !tc_check_expr(s, expr->composable)) {
    valid = false; // No need to error here; composable typecheck should report
  }

  return valid ? def->return_type : tc_error();
}

static Type *_import_type(Symbols *s, const ImportExpr *expr) {
  tc_debug_log("Import", &expr->meta.loc);

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
  tc_debug_log("Env", &expr->meta.loc);

  if (expr->default_value) {
    Type *default_value_type = tc_check_expr(s, expr->default_value);
    if (!_is_builtin(default_value_type, T_STRING)) {
      return tc_report_error_fmt(
        &expr->default_value->meta.loc,
        "invalid default value for env: expected string, got %s",
        tc_print_type(default_value_type)
      );
    }
  }
  return _builtin_type(T_STRING);
}

static Type *tc_check_block(Symbols *s, const BlockExpr *expr) {
  tc_debug_log("Block", &expr->meta.loc);

  Type **block_types = tc_new_array(Type *, expr->len);
  u32 type_count = 0;
  bool homogenous = true;

  for (u32 i = 0; i < expr->len; i++) {
    Type *stmt_type = tc_check_stmt(s, expr->statements[i]);
    if (stmt_type) {
      if (type_count > 0 && !tc_compare_types(s, block_types[0], stmt_type)) {
        homogenous = false;
      }
      block_types[type_count++] = stmt_type;
    }
  }

  if (type_count == 0) {
    return tc_report_error_fmt(&expr->meta.loc, "empty block expression");
  } else if (type_count == 1) {
    return block_types[0];
  }

  return homogenous
    ? _vector_of(block_types[0], type_count)
    : _tuple_of(block_types, type_count);
}

static Type *_block_type(Symbols *s, const BlockExpr *expr) {
  _push_scope(expr->scope);
  Type *type = tc_check_block(s, expr);
  _pop_scope();
  return type;
}

static Type *tc_check_expr(Symbols *s, const Expr *expr) {
  Type *type = NULL;
  switch (expr->type) {
    case EXPR_LITERAL: type = _literal_type(s, expr->literal); break;
    case EXPR_VARIABLE: type = _variable_type(s, expr->variable); break;
    case EXPR_UNARY: type = _unary_type(s, expr->unary); break;
    case EXPR_BINARY: type = _binary_type(s, expr->binary); break;
    case EXPR_ASSIGNMENT: type = _assignment_type(s,expr->assignment); break;
    case EXPR_IF: type = _if_expr_type(s, expr->if_expr); break;
    case EXPR_FOR: type = _for_expr_type(s, expr->for_expr); break;
    case EXPR_BLOCK: type = _block_type(s, expr->block); break;
    case EXPR_FUNCTION_CALL: type = _function_call_type(s, expr->function_call); break;
    case EXPR_IMPORT: type = _import_type(s, expr->import); break;
    case EXPR_ENV: type = _env_type(s, expr->env); break;
    case EXPR_GROUP: type = tc_check_expr(s, expr->group->inner_expr); break;
  }

  tc_debug_log("Expr", &expr->meta.loc);
  logger_log(sa->tc_logger, LOG_DEBUG, "\t -> %s", tc_print_type(type));
  return type;
}

static void _check_decl(Symbols *s, const DeclarationStmt *decl) {
  tc_debug_log("Decl", &decl->meta.loc);

  Type *value_type = tc_check_expr(s, decl->right);
  // Fail silently; assume error has been reported upstream
  if (_is_error(value_type)) return;

  if (decl->left_count == 1) {
    Type *decl_type = or_default(decl->type, value_type);
    if (!tc_accepts_type(s, decl_type, value_type)) {
      tc_report_error_fmt(
        &decl->right->meta.loc,
        "%s is not assignable to %s",
        tc_print_type(value_type), tc_print_type(decl_type)
      );
      return;
    }

    tc_define_symbol(s, decl->left[0], _symbol_of(decl_type, false));
  } else {
    switch (value_type->type) {
      case T_LIST: {
        Type *decl_type = or_default(decl->type, value_type->list->item_type);
        if (!tc_accepts_type(s, decl_type, value_type->list->item_type)) {
          tc_report_error_fmt(
            &decl->right->meta.loc,
            "%s is not assignable to %s",
            tc_print_type(value_type->list->item_type), tc_print_type(decl_type)
          );
          return;
        }

        for (u32 i = 0; i < decl->left_count; i++) {
          tc_define_symbol(s, decl->left[i], _symbol_of(decl_type, false));
        }
        break;
      }
      case T_VECTOR: {
        Type *decl_type = or_default(decl->type, value_type->vec->item_type);
        if (!tc_accepts_type(s, decl_type, value_type->vec->item_type)) {
          tc_report_error_fmt(
            &decl->right->meta.loc,
            "%s is not assignable to %s",
            tc_print_type(value_type->vec->item_type), tc_print_type(decl_type)
          );
          return;
        }
        if (decl->left_count > value_type->vec->len) {
          tc_report_error_fmt(
            &decl->left[value_type->vec->len]->loc,
            "too many variables for type %s",
            tc_print_type(value_type)
          );
          return;
        }

        for (u32 i = 0; i < decl->left_count; i++) {
          tc_define_symbol(s, decl->left[i], _symbol_of(decl_type, false));
        }
        break;
      }
      case T_TUPLE: {
        bool valid = true;
        for (u32 i = 0; i < decl->left_count; i++) {
          if (i >= value_type->tuple->len) {
            tc_report_error_fmt(
              &decl->left[value_type->vec->len]->loc,
              "too many variables for type %s",
              tc_print_type(value_type)
            );
            return;
          }

          Type *decl_type = or_default(decl->type, value_type->tuple->types[i]);
          if (!tc_accepts_type(s, decl_type, value_type->tuple->types[i])) {
            tc_report_error_fmt(
              &decl->right->meta.loc,
              "%s is not assignable to %s",
              tc_print_type(value_type->tuple->types[i]), tc_print_type(decl_type)
            );
            valid = false;
          }
        }
        if (!valid) return;

        for (u32 i = 0; i < decl->left_count; i++) {
          Type *decl_type = or_default(decl->type, value_type->tuple->types[i]);
          tc_define_symbol(s, decl->left[i], _symbol_of(decl_type, false));
        }
        break;
      }
      default: {
        tc_report_error_fmt(
          &decl->right->meta.loc,
          "%s does not allow decomposition", tc_print_type(value_type)
        );
      }
    }
  }
}

static void _check_constant(Symbols *s, const ConstantStmt *constant) {
  Type *value_type = tc_check_expr(s, constant->right);

  Type *decl_type = or_default(constant->type, value_type);
  if (!tc_accepts_type(s, decl_type, value_type)) {
    tc_report_error_fmt(
      &constant->right->meta.loc,
      "%s is not assignable to %s",
      tc_print_type(value_type), tc_print_type(decl_type)
    );
    return;
  }

  tc_define_symbol(s, constant->left, _symbol_of(decl_type, true));
}

static void _check_function_def(Symbols *s, const FunctionDef *def) {
  _push_scope(def->scope);

  for (u32 i = 0; i < def->params_len; i++) {
    if (def->params[i]->default_value) {
      Type *dv_type = tc_check_expr(s, def->params[i]->default_value);
      if (!tc_accepts_type(s, def->params[i]->type, dv_type)) {
        tc_report_error_fmt(
          &def->params[i]->default_value->meta.loc,
          "%s is not assignable to %s",
          tc_print_type(dv_type), tc_print_type(def->params[i]->type)
        );
        continue;
      }
    }

    Symbol *param = _symbol_of(def->params[i]->type, true);
    tc_define_symbol(s, def->params[i]->name, param);
  }

  Type *body_type = tc_check_block(s, def->body);
  if (!tc_accepts_type(s, def->return_type, body_type)) {
    tc_report_error_fmt(
      &def->return_type->meta.loc,
      "invalid function return type: %s is not assignable to %s",
      tc_print_type(body_type), tc_print_type(def->return_type)
    );
  }

  _pop_scope();
}

static void _check_export(Symbols *s, const ExportStmt *export) {
  for (u32 i = 0; i < export->len; i++) {
    // TODO
  }
}

static Type *tc_check_stmt(Symbols *s, const Stmt *stmt) {
  tc_debug_log("Stmt", &stmt->meta.loc);

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

static Type *tc_check_ast(Symbols *s, const Program *ast) {
  _push_scope(ast->scope);

  Type **types = tc_new_array(Type *, ast->len);
  u32 type_count = 0;
  bool homogenous = true;

  for (u32 i = 0; i < ast->len; i++) {
    Type *stmt_type = tc_check_stmt(s, ast->statements[i]);
    if (stmt_type) {
      if (type_count > 0 && !tc_compare_types(s, types[0], stmt_type)) {
        homogenous = false;
      }
      types[type_count++] = stmt_type;
    }
  }

  _pop_scope();

  if (type_count == 0) {
    return _builtin_type(T_VOID);
  } else if (type_count == 1) {
    return types[0];
  }

  return homogenous
    ? _vector_of(types[0], type_count)
    : _tuple_of(types, type_count);
}

CompilationStatus sa_build_tables_and_typecheck(i32 file_idx) {
  cs->current_file = file_idx;

  Program *ast = cs->asts[file_idx];
  assert(ast != NULL); // If we got to this point without an AST we have a bug

  Symbols *sym = cs->symbol_data[file_idx];
  assert(sym != NULL); // If we got to this point without a symbol table we have a bug
  sym->symbols = new(SymbolTable);
  map_init(sym->symbols, sk_hash, sk_compare);
  map_manage_keys(sym->symbols, sk_clone, sk_free);

  dyn_array_clear(sa->stack);
  mem_dyn_arena_free_all(sa->tc_arena);
  tc_check_ast(sym, ast);
  assert(dyn_array_size(sa->stack) == 0); // Unbalanced stack is a bug

  cs->current_file = -1;
  return STATUS_SUCCEEDED;
}

void sa_free_symbols(Symbols *s) {
  if (!s) return;

  if (s->types) { map_free(s->types); free(s->types); }
  if (s->functions) { map_free(s->functions); free(s->functions); }
  if (s->symbols) { map_free(s->symbols); free(s->symbols); }
  free(s);
}
