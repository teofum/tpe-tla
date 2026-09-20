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

// -----------------------------------------------------------------------------

LiteralExpr *ast_literal_integer(i64 i) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_INTEGER,
    .int_value = i,
  };

  return literal;
}

LiteralExpr *ast_literal_float(f64 f) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_FLOAT,
    .float_value = f,
  };

  return literal;
}

LiteralExpr *ast_literal_string(str s) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_STRING,
    .string_value = s,
  };

  return literal;
}

LiteralExpr *ast_literal_boolean(bool b) {
  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_BOOL,
    .bool_value = b,
  };

  return literal;
}

// -----------------------------------------------------------------------------

UnaryExpr *ast_unary(TokenLabel op, Expr *expr) {
  UnaryExpr *unary = new(UnaryExpr);
  *unary = (UnaryExpr){
    .op = op,
    .expr = expr,
  };

  return unary;
}

BinaryExpr *ast_binary(Expr *left, TokenLabel op, Expr *right) {
  BinaryExpr *binary = new(BinaryExpr);
  *binary = (BinaryExpr){
    .left = left,
    .op = op,
    .right = right,
  };

  return binary;
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
    // TODO other types
  }

  free(expr);
}

void ast_free_literal(LiteralExpr *literal) {
  if (!literal) return;

  if (literal->type == L_STRING) str_free(literal->string_value);
  free(literal);
}

void ast_free_unary(UnaryExpr *unary) {
  if (!unary) return;

  ast_free_expr(unary->expr);
  free(unary);
}

void ast_free_binary(BinaryExpr *binary) {
  if (!binary) return;

  ast_free_expr(binary->left);
  ast_free_expr(binary->right);
  free(binary);
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
