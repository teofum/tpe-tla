#include <stdlib.h>
#include <support/types.h>
#include <support/util.h>
#include <support/str.h>

#include "ast.h"

// -----------------------------------------------------------------------------

#define AST_STMT_FUNC(f_name, T, stmt_t, p_name)  \
Stmt *f_name(T *p_name) {                         \
  Stmt *stmt = new(Stmt);                         \
  *stmt = (Stmt){                                 \
    .type = stmt_t,                               \
    .p_name = p_name,                             \
  };                                              \
  return stmt;                                    \
}                                                 \

AST_STMT_FUNC(ast_stmt_expr, Expr, STMT_EXPR, expr)
AST_STMT_FUNC(ast_stmt_decl, DeclarationStmt, STMT_DECLARATION, decl)
AST_STMT_FUNC(ast_stmt_alias, TypeAliasStmt, STMT_TYPE_ALIAS, type_alias)

// -----------------------------------------------------------------------------

DeclarationStmt *ast_declaration(TokenMeta *id, Type *type, Expr *expr) {
  DeclarationStmt *decl = new(DeclarationStmt);
  *decl = (DeclarationStmt){
    .left = id,
    .right = expr,
    .type = type,
  };

  return decl;
}

TypeAliasStmt *ast_type_alias(TokenMeta *id, Type *type) {
  TypeAliasStmt *alias = new(TypeAliasStmt);
  *alias = (TypeAliasStmt){
    .alias = id,
    .type = type,
  };

  return alias;
}

// -----------------------------------------------------------------------------

#define AST_EXPR_FUNC(f_name, T, expr_t, p_name)  \
Expr *f_name(T *p_name) {                         \
  Expr *expr = new(Expr);                         \
  *expr = (Expr){                                 \
    .type = expr_t,                               \
    .p_name = p_name,                             \
  };                                              \
  return expr;                                    \
}                                                 \

AST_EXPR_FUNC(ast_expr_literal, LiteralExpr, EXPR_LITERAL, literal)
AST_EXPR_FUNC(ast_expr_variable, VariableExpr, EXPR_VARIABLE, variable)
AST_EXPR_FUNC(ast_expr_unary, UnaryExpr, EXPR_UNARY, unary)
AST_EXPR_FUNC(ast_expr_binary, BinaryExpr, EXPR_BINARY, binary)
AST_EXPR_FUNC(ast_expr_group, GroupExpr, EXPR_GROUP, group)
AST_EXPR_FUNC(ast_expr_assignment, AssignmentExpr, EXPR_ASSIGNMENT, assignment)
AST_EXPR_FUNC(ast_expr_if, IfExpr, EXPR_IF, if_expr)
AST_EXPR_FUNC(ast_expr_for, ForExpr, EXPR_FOR, for_expr)
AST_EXPR_FUNC(ast_expr_block, BlockExpr, EXPR_BLOCK, block)

// -----------------------------------------------------------------------------

#define AST_LITERAL_FUNC(f_name, T, literal_t, p_name)  \
LiteralExpr *f_name(T *p_name) {                        \
  LiteralExpr *literal = new(LiteralExpr);              \
  *literal = (LiteralExpr){                             \
    .type = literal_t,                                  \
    .p_name = p_name,                                   \
  };                                                    \
  return literal;                                       \
}                                                       \

AST_LITERAL_FUNC(ast_literal_integer, IntegerLiteral, L_INTEGER, integer)
AST_LITERAL_FUNC(ast_literal_float, FloatLiteral, L_FLOAT, floating)
AST_LITERAL_FUNC(ast_literal_string, StringLiteral, L_STRING, string)
AST_LITERAL_FUNC(ast_literal_boolean, BooleanLiteral, L_BOOL, boolean)

// -----------------------------------------------------------------------------

VariableExpr *ast_variable_named(TokenMeta *id) {
  NamedVariable *named_var = new(NamedVariable);
  *named_var = (NamedVariable){ .meta = id };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .type = V_NAMED,
    .named = named_var,
  };

  return var;
}

VariableExpr *ast_variable_struct_member(VariableExpr *struct_expr, TokenMeta *id) {
  StructMemberVariable *struct_var = new(StructMemberVariable);
  *struct_var = (StructMemberVariable){
    .struct_expr = struct_expr,
    .meta = id,
  };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .type = V_STRUCT_MEMBER,
    .struct_member = struct_var,
  };

  return var;
}

VariableExpr *ast_variable_indexed(VariableExpr *container, Expr *index) {
  IndexedVariable *idx_var = new(IndexedVariable);
  *idx_var = (IndexedVariable){
    .container = container,
    .index = index,
  };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .type = V_INDEX,
    .indexed = idx_var,
  };

  return var;
}

// -----------------------------------------------------------------------------

UnaryExpr *ast_unary(TokenMeta *op, Expr *expr) {
  UnaryExpr *unary = new(UnaryExpr);
  *unary = (UnaryExpr){
    .op = op,
    .expr = expr,
  };

  return unary;
}

BinaryExpr *ast_binary(Expr *left, TokenMeta *op, Expr *right) {
  BinaryExpr *binary = new(BinaryExpr);
  *binary = (BinaryExpr){
    .left = left,
    .op = op,
    .right = right,
  };

  return binary;
}

GroupExpr *ast_group(Expr *expr) {
  GroupExpr *group = new(GroupExpr);
  *group = (GroupExpr){ .inner_expr = expr };

  return group;
}

AssignmentExpr *ast_assignment(VariableExpr *left, Expr *right) {
  AssignmentExpr *assign = new(AssignmentExpr);
  *assign = (AssignmentExpr){
    .left = left,
    .right = right,
  };

  return assign;
}

IfExpr *ast_if(Expr *condition, Expr *true_branch, Expr *false_branch) {
  IfExpr *if_expr = new(IfExpr);
  *if_expr = (IfExpr){
    .condition = condition,
    .true_branch = true_branch,
    .false_branch = false_branch,
  };

  return if_expr;
}

ForExpr *ast_for(TokenMeta *var, TokenMeta *idx, Expr *iterable, Expr *body) {
  ForExpr *for_expr = new(ForExpr);
  *for_expr = (ForExpr){
    .var_id = var,
    .idx_id = idx,
    .iterable = iterable,
    .body = body,
  };

  return for_expr;
}

BlockExpr *ast_block(StmtList *statements, Expr *final) {
  u32 len = statements ? statements->len + 1 : 1;

  BlockExpr* block = new(BlockExpr);
  *block = (BlockExpr){
    .len = len,
    .statements = new_array(Stmt *, len),
  };

  StmtList *tail = statements;
  for (u32 i = 0; tail != NULL && i < len - 1; i++) {
    block->statements[len - i - 2] = tail->head;
    tail = tail->tail;
  }

  block->statements[block->len - 1] = ast_stmt_expr(final);

  // We consumed the expression list, so free its memory without freeing the
  // actual expressions, which we now own
  ast_free_stmt_list(statements, false);
  return block;
}

// -----------------------------------------------------------------------------

Type *ast_type_named(TokenMeta *id) {
  NamedType *named_type = new(NamedType);
  *named_type = (NamedType){ .meta = id };

  Type *type = new(Type);
  *type = (Type){ .type = T_NAMED, .named = named_type };

  return type;
}

Type *ast_type_list(Type *item_type) {
  ListType *list = new(ListType);
  *list = (ListType){ .item_type = item_type };

  Type *type = new(Type);
  *type = (Type){ .type = T_LIST, .list = list };

  return type;
}

Type *ast_type_map(Type *key_type, Type *value_type) {
  MapType *map = new(MapType);
  *map = (MapType){
    .key_type = key_type,
    .value_type = value_type,
  };

  Type *type = new(Type);
  *type = (Type){ .type = T_MAP, .map = map };

  return type;
}

Type *ast_type_nil() {
  Type *type = new(Type);
  *type = (Type){ .type = T_NIL };

  return type;
}


// -----------------------------------------------------------------------------

StmtList *ast_stmt_list(Stmt *head, StmtList *tail) {
  StmtList* list = new(StmtList);
  list->head = head;
  if (tail) {
    list->tail = tail;
    list->len = tail->len + 1;
  } else {
    list->tail = NULL;
    list->len = 1;
  }

  return list;
}

Program *ast_program(StmtList *statements) {
  Program* prog = new(Program);
  *prog = (Program){
    .len = statements->len,
    .statements = new_array(Stmt *, statements->len),
  };

  StmtList *tail = statements;
  for (u32 i = 0; tail != NULL && i < prog->len; i++) {
    prog->statements[prog->len - i - 1] = tail->head;
    tail = tail->tail;
  }

  // We consumed the expression list, so free its memory without freeing the
  // actual expressions, which we now own
  ast_free_stmt_list(statements, false);
  return prog;
}

// -----------------------------------------------------------------------------

void ast_free_stmt(Stmt *stmt) {
  if (!stmt) return;

  switch (stmt->type) {
    case STMT_EXPR: ast_free_expr(stmt->expr); break;
    case STMT_DECLARATION: ast_free_decl(stmt->decl); break;
    case STMT_TYPE_ALIAS: ast_free_alias(stmt->type_alias); break;
  }

  free(stmt);
}

void ast_free_decl(DeclarationStmt *decl) {
  if (!decl) return;

  ast_free_meta(decl->left);
  ast_free_expr(decl->right);
  ast_free_type(decl->type);
  free(decl);
}

void ast_free_alias(TypeAliasStmt *alias) {
  if (!alias) return;

  ast_free_meta(alias->alias);
  ast_free_type(alias->type);
  free(alias);
}

void ast_free_expr(Expr *expr) {
  if (!expr) return;

  switch (expr->type) {
    case EXPR_LITERAL: ast_free_literal(expr->literal); break;
    case EXPR_VARIABLE: ast_free_variable(expr->variable); break;
    case EXPR_UNARY: ast_free_unary(expr->unary); break;
    case EXPR_BINARY: ast_free_binary(expr->binary); break;
    case EXPR_GROUP: ast_free_group(expr->group); break;
    case EXPR_ASSIGNMENT: ast_free_assignment(expr->assignment); break;
    case EXPR_IF: ast_free_if(expr->if_expr); break;
    case EXPR_FOR: ast_free_for(expr->for_expr); break;
    case EXPR_BLOCK: ast_free_block(expr->block); break;
  }

  free(expr);
}

void ast_free_literal(LiteralExpr *literal) {
  if (!literal) return;

  switch (literal->type) {
    case L_INTEGER: ast_free_int_literal(literal->integer); break;
    case L_FLOAT: ast_free_float_literal(literal->floating); break;
    case L_STRING: ast_free_string_literal(literal->string); break;
    case L_BOOL: ast_free_bool_literal(literal->boolean); break;
  }
  free(literal);
}

void ast_free_variable(VariableExpr *var) {
  if (!var) return;

  switch (var->type) {
    case V_NAMED: ast_free_named_variable(var->named); break;
    case V_STRUCT_MEMBER: ast_free_struct_member_variable(var->struct_member); break;
    case V_INDEX: ast_free_indexed_variable(var->indexed); break;
  }
  free(var);
}

void ast_free_unary(UnaryExpr *unary) {
  if (!unary) return;

  ast_free_expr(unary->expr);
  ast_free_meta(unary->op);
  free(unary);
}

void ast_free_binary(BinaryExpr *binary) {
  if (!binary) return;

  ast_free_expr(binary->left);
  ast_free_expr(binary->right);
  ast_free_meta(binary->op);
  free(binary);
}

void ast_free_group(GroupExpr *group) {
  if (!group) return;

  ast_free_expr(group->inner_expr);
  free(group);
}

void ast_free_assignment(AssignmentExpr *assign) {
  if (!assign) return;

  ast_free_variable(assign->left);
  ast_free_expr(assign->right);
  free(assign);
}

void ast_free_if(IfExpr *if_expr) {
  if (!if_expr) return;

  ast_free_expr(if_expr->condition);
  ast_free_expr(if_expr->true_branch);
  ast_free_expr(if_expr->false_branch);
  free(if_expr);
}

void ast_free_for(ForExpr *for_expr) {
  if (!for_expr) return;

  ast_free_meta(for_expr->var_id);
  ast_free_meta(for_expr->idx_id);
  ast_free_expr(for_expr->body);
  ast_free_expr(for_expr->iterable);
  free(for_expr);
}

void ast_free_block(BlockExpr *block) {
  if (!block) return;

  for (u32 i = 0; i < block->len; i++) {
    ast_free_stmt(block->statements[i]);
  }
  free(block->statements);
  free(block);
}

void ast_free_int_literal(IntegerLiteral *l) {
  if (!l) return;

  ast_free_meta(l->meta);
  free(l);
}

void ast_free_float_literal(FloatLiteral *l) {
  if (!l) return;

  ast_free_meta(l->meta);
  free(l);
}

void ast_free_string_literal(StringLiteral *l) {
  if (!l) return;

  str_free(l->value);
  ast_free_meta(l->meta);
  free(l);
}

void ast_free_bool_literal(BooleanLiteral *l) {
  if (!l) return;

  ast_free_meta(l->meta);
  free(l);
}

void ast_free_named_variable(NamedVariable *v) {
  if (!v) return;

  ast_free_meta(v->meta);
  free(v);
}

void ast_free_struct_member_variable(StructMemberVariable *v) {
  if (!v) return;

  ast_free_variable(v->struct_expr);
  ast_free_meta(v->meta);
  free(v);
}

void ast_free_indexed_variable(IndexedVariable *v) {
  if (!v) return;

  ast_free_variable(v->container);
  ast_free_expr(v->index);
  free(v);
}

void ast_free_type(Type *t) {
  if (!t) return;

  switch (t->type) {
    case T_NAMED: ast_free_named_type(t->named); break;
    case T_LIST: ast_free_list_type(t->list); break;
    case T_MAP: ast_free_map_type(t->map); break;
    case T_NIL: break;
  }
  free(t);
}

void ast_free_named_type(NamedType *t) {
  if (!t) return;

  ast_free_meta(t->meta);
  free(t);
}

void ast_free_list_type(ListType *t) {
  if (!t) return;

  ast_free_type(t->item_type);
  free(t);
}

void ast_free_map_type(MapType *t) {
  if (!t) return;

  ast_free_type(t->key_type);
  ast_free_type(t->value_type);
  free(t);
}

void ast_free_meta(TokenMeta *meta) {
  if (!meta) return;

  free(meta->lexeme);
  free(meta);
}

void ast_free_stmt_list(StmtList *list, bool free_stmts) {
  if (!list) return;

  if (free_stmts) ast_free_stmt(list->head);
  ast_free_stmt_list(list->tail, free_stmts);

  free(list);
}

void ast_free_program(Program *program) {
  if (!program) return;

  for (u32 i = 0; i < program->len; i++) {
    ast_free_stmt(program->statements[i]);
  }
  free(program->statements);
  free(program);
}
