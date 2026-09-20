#include <ast/ast.h>
#include <frontend/frontend.h>
#include <support/types.h>
#include <support/util.h>

#include "bison_actions.h"
#include "support/str.h"

Expr *parse_literal_expr(LiteralExpr *literal) {
  Expr *expr = new(Expr);
  *expr = (Expr){
    .type = EXPR_LITERAL,
    .literal = literal,
  };

  fe_parser_log(LOG_DEBUG, "Literal Expr");
  return expr;
}

LiteralExpr *parse_integer_literal(i64 i) {
  LiteralExpr* literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .type = L_INTEGER,
    .int_value = i,
  };

  fe_parser_log(LOG_DEBUG, "Integer Literal %lld", i);
  return literal;
}

LiteralExpr *parse_float_literal(f64 f) {
 LiteralExpr* literal = new(LiteralExpr);
 *literal = (LiteralExpr){
   .type = L_FLOAT,
   .float_value = f,
 };

 fe_parser_log(LOG_DEBUG, "Float Literal %f", f);
 return literal;
}

LiteralExpr *parse_string_literal(str s) {
 LiteralExpr* literal = new(LiteralExpr);
 *literal = (LiteralExpr){
   .type = L_STRING,
   .string_value = s,
 };

 char *cstring = str_to_cstring(s);
 fe_parser_log(LOG_DEBUG, "String Literal '%s'", cstring);
 free(cstring);
 return literal;
}

LiteralExpr *parse_boolean_literal(bool b) {
 LiteralExpr* literal = new(LiteralExpr);
 *literal = (LiteralExpr){
   .type = L_BOOL,
   .bool_value = b,
 };

 fe_parser_log(LOG_DEBUG, "Boolean Literal %s", b ? "true" : "false");
 return literal;
}

ExprList *parse_expr_list(Expr *head, ExprList *tail) {
  ExprList* list = new(ExprList);
  list->head = head;
  if (tail) {
    list->tail = tail;
    list->len = tail->len + 1;
  } else {
    list->tail = NULL;
    list->len = 1;
  }

  fe_parser_log(LOG_DEBUG, "Expr List (len=%u)", list->len);
  return list;
}

Program *parse_program(ExprList *exprs) {
  Program* prog = new(Program);
  *prog = (Program){
    .len = exprs->len,
    .exprs = new_array(Expr, exprs->len),
  };

  for (u32 i = 0; exprs != NULL && i < exprs->len; i++) {
    prog->exprs[i] = *exprs->head;
    exprs = exprs->tail;
  }

  fe_parser_log(LOG_DEBUG, "Program (len=%u)", prog->len);
  return prog;
}
