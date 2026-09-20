#include <stdlib.h>
#include <support/types.h>
#include <support/util.h>

#include "ast.h"
#include "support/str.h"

Expr *ast_expr_literal(LiteralExpr *literal) {
  Expr *expr = new(Expr);
  *expr = (Expr){
    .type = EXPR_LITERAL,
    .literal = literal,
  };

  return expr;
}

LiteralExpr *ast_literal_integer(i64 i) {
  LiteralExpr* literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_INTEGER,
    .int_value = i,
  };

  return literal;
}

LiteralExpr *ast_literal_float(f64 f) {
  LiteralExpr* literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_FLOAT,
    .float_value = f,
  };

  return literal;
}

LiteralExpr *ast_literal_string(str s) {
  LiteralExpr* literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_STRING,
    .string_value = s,
  };

  return literal;
}

LiteralExpr *ast_literal_boolean(bool b) {
  LiteralExpr* literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_BOOL,
    .bool_value = b,
  };

  return literal;
}


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

void ast_free_expr(Expr *expr) {
  if (!expr) return;

  switch (expr->type) {
    case EXPR_LITERAL: ast_free_literal(expr->literal);
    // TODO other types
  }

  free(expr);
}

void ast_free_literal(LiteralExpr *literal) {
  if (!literal) return;

  if (literal->type == L_STRING) str_free(literal->string_value);
  free(literal);
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
