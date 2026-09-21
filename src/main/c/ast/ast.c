#include <stdlib.h>
#include <support/types.h>
#include <support/util.h>
#include <support/str.h>

#include "ast.h"

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
AST_EXPR_FUNC(ast_expr_decl, DeclarationExpr, EXPR_DECLARATION, declaration)

// -----------------------------------------------------------------------------

LiteralExpr *ast_literal_integer(IntegerLiteral *i) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_INTEGER,
    .integer = i,
  };

  return literal;
}

LiteralExpr *ast_literal_float(FloatLiteral *f) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_FLOAT,
    .floating = f,
  };

  return literal;
}

LiteralExpr *ast_literal_string(StringLiteral *s) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_STRING,
    .string = s,
  };

  return literal;
}

LiteralExpr *ast_literal_boolean(BooleanLiteral *b) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_BOOL,
    .boolean = b,
  };

  return literal;
}

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

DeclarationExpr *ast_declaration(TokenMeta *id, Type *type, Expr *expr) {
  DeclarationExpr *decl = new(DeclarationExpr);
  *decl = (DeclarationExpr){
    .left = id,
    .right = expr,
    .type = type,
  };

  return decl;
}

// -----------------------------------------------------------------------------

Type *ast_type_named(TokenMeta *id) {
  NamedType *named_type = new(NamedType);
  *named_type = (NamedType){ .meta = id };

  Type *type = new(Type);
  *type = (Type){
    .type = T_NAMED,
    .named = named_type,
  };

  return type;
}

// -----------------------------------------------------------------------------

ExprList *ast_expr_list(Expr *head, ExprList *tail) {
  ExprList* list = new(ExprList);
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

Program *ast_program(ExprList *exprs) {
  Program* prog = new(Program);
  *prog = (Program){
    .len = exprs->len,
    .exprs = new_array(Expr *, exprs->len),
  };

  ExprList *tail = exprs;
  for (u32 i = 0; tail != NULL && i < prog->len; i++) {
    prog->exprs[i] = tail->head;
    tail = tail->tail;
  }

  // We consumed the expression list, so free its memory without freeing the
  // actual expressions, which we now own
  ast_free_expr_list(exprs, false);
  return prog;
}

// -----------------------------------------------------------------------------

void ast_free_expr(Expr *expr) {
  if (!expr) return;

  switch (expr->type) {
    case EXPR_LITERAL: ast_free_literal(expr->literal); break;
    case EXPR_VARIABLE: ast_free_variable(expr->variable); break;
    case EXPR_UNARY: ast_free_unary(expr->unary); break;
    case EXPR_BINARY: ast_free_binary(expr->binary); break;
    case EXPR_GROUP: ast_free_group(expr->group); break;
    case EXPR_ASSIGNMENT: ast_free_assignment(expr->assignment); break;
    case EXPR_DECLARATION: ast_free_decl(expr->declaration); break;
    // TODO other types
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

void ast_free_decl(DeclarationExpr *decl) {
  if (!decl) return;

  ast_free_meta(decl->left);
  ast_free_expr(decl->right);
  ast_free_type(decl->type);
  free(decl);
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
  }
  free(t);
}

void ast_free_named_type(NamedType *t) {
  if (!t) return;

  ast_free_meta(t->meta);
  free(t);
}

void ast_free_meta(TokenMeta *meta) {
  if (!meta) return;

  free(meta->lexeme);
  free(meta);
}

void ast_free_expr_list(ExprList *list, bool free_exprs) {
  if (!list) return;

  if (free_exprs) ast_free_expr(list->head);
  ast_free_expr_list(list->tail, free_exprs);

  free(list);
}

void ast_free_program(Program *program) {
  if (!program) return;

  for (u32 i = 0; i < program->len; i++) {
    ast_free_expr(program->exprs[i]);
  }
  free(program->exprs);
  free(program);
}
