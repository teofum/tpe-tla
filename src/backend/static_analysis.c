#include <assert.h>
#include <string.h>

#include <ast/ast.h>
#include <ast/symbols.h>
#include <error/error.h>
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
  dyn_array *scope_stack;
  u32 next_scope_id;
} StaticAnalyzer;

static CompilerState *cs = NULL;
static StaticAnalyzer *sa = NULL;

static u32 _new_scope() { return sa->next_scope_id++; }
static u32 _push_scope(u32 scope) {
  dyn_array_push(sa->scope_stack, &scope);
  return scope;
}
static void _pop_scope() {
  dyn_array_remove_unordered(sa->scope_stack, dyn_array_size(sa->scope_stack) - 1);
}
static u32 _peek_scope() {
  return *(u32 *)dyn_array_get(sa->scope_stack, dyn_array_size(sa->scope_stack) - 1);
}

void sa_init(CompilerState *compiler_state) {
  cs = compiler_state;

  sa = new(StaticAnalyzer);
  sa->next_scope_id = 0;
  sa->scope_stack = new_dyn_array(u32);
  sa->sym_logger = logger_create("Symbols", stderr, cs->options.symbols_log_level);
  sa->tc_logger = logger_create("Typecheck", stderr, cs->options.typecheck_log_level);
}

void sa_shutdown() {
  if (!sa) return;

  if (sa->scope_stack) dyn_array_free(sa->scope_stack);
  if (sa->sym_logger) logger_free(sa->sym_logger);
  if (sa->tc_logger) logger_free(sa->tc_logger);
  free(sa);

  sa = NULL;
  cs = NULL;
}

static void _static_tables_stmt(Symbols *s, Stmt *stmt);
static void _static_tables_expr(Symbols *s, Expr *expr);
static void _static_tables_block(Symbols *s, BlockExpr *block);
static void _static_tables_literal(Symbols *s, LiteralExpr *literal);
static void _static_tables_variable(Symbols *s, VariableExpr *var);

static void _static_tables_add_type(Symbols *s, TypeAliasStmt *type_alias) {
  SymbolKey key = { .name = type_alias->alias->lexeme, .scope = _peek_scope() };
  logger_log(sa->sym_logger, LOG_DEBUG, "Scope %u: Type %s", key.scope, key.name);
  if (!map_insert(s->types, &key, type_alias)) {
    logger_log(sa->sym_logger, LOG_ERROR, "Scope %u: Redefinition of type %s", key.scope, key.name);

    TypeAliasStmt *prev = map_get(s->types, &key);
    RedefinitionError *error = new(RedefinitionError);
    *error = (RedefinitionError){
      .object_type = "type",
      .object_name = strdup(key.name),
      .previous_definition = prev->meta.loc
    };
    err_report_redefinition(error, &type_alias->meta.loc, ERR_ERROR);
  }
}

static void _static_tables_add_function(Symbols *s, FunctionDef *function) {
  _static_tables_block(s, function->body);

  SymbolKey key = { .name = function->name->lexeme, .scope = _peek_scope() };
  logger_log(sa->sym_logger, LOG_DEBUG, "Scope %u: Function %s", key.scope, key.name);
  if (!map_insert(s->functions, &key, function)) {
    logger_log(sa->sym_logger, LOG_ERROR, "Scope %u: Redefinition of function %s", key.scope, key.name);

    FunctionDef *prev = map_get(s->functions, &key);
    RedefinitionError *error = new(RedefinitionError);
    *error = (RedefinitionError){
      .object_type = "function",
      .object_name = strdup(key.name),
      .previous_definition = prev->meta.loc
    };
    err_report_redefinition(error, &function->meta.loc, ERR_ERROR);
  }
}

static void _static_tables_block(Symbols *s, BlockExpr *block) {
  block->scope = _push_scope(_new_scope());
  logger_log(sa->sym_logger, LOG_DEBUG, ">>> Scope %u [%d:%d]", block->scope, block->meta.loc.first_line, block->meta.loc.first_column);
  for (u32 i = 0; i < block->len; i++) {
    _static_tables_stmt(s, block->statements[i]);
  }
  logger_log(sa->sym_logger, LOG_DEBUG, "<<< Scope %u [%d:%d]", block->scope, block->meta.loc.last_line, block->meta.loc.last_column);
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
      _static_tables_expr(s, literal->short_list->count);
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
    case V_STRUCT_MEMBER: _static_tables_variable(s, var->struct_member->struct_expr); break;
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
      if (expr->function_call->composable) _static_tables_expr(s, expr->function_call->composable);
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
      if (expr->if_expr->false_branch) _static_tables_expr(s, expr->if_expr->false_branch);
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

  dyn_array_clear(sa->scope_stack);
  _build_static_tables_ast(sym, ast);
  assert(dyn_array_size(sa->scope_stack) == 0); // Unbalanced stack is a bug

  cs->current_file = -1;
  return STATUS_SUCCEEDED;
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
