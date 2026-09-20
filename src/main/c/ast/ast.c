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
AST_EXPR_FUNC(ast_expr_unary, UnaryExpr, EXPR_UNARY, unary)
AST_EXPR_FUNC(ast_expr_binary, BinaryExpr, EXPR_BINARY, binary)
AST_EXPR_FUNC(ast_expr_group, GroupExpr, EXPR_GROUP, group)

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
    case EXPR_UNARY: ast_free_unary(expr->unary); break;
    case EXPR_BINARY: ast_free_binary(expr->binary); break;
    case EXPR_GROUP: ast_free_group(expr->group); break;
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
