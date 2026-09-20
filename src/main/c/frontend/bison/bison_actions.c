#include <ast/ast.h>
#include <frontend/frontend.h>
#include <support/types.h>
#include <support/util.h>

#include "bison_actions.h"
#include "support/str.h"

Expr *parse_literal_expr(LiteralExpr *literal) {
  fe_parser_log(LOG_DEBUG, "Literal Expr");
  return ast_expr_literal(literal);
}

LiteralExpr *parse_integer_literal(i64 i) {
  fe_parser_log(LOG_DEBUG, "Integer Literal %lld", i);
  return ast_literal_integer(i);
}

LiteralExpr *parse_float_literal(f64 f) {
  fe_parser_log(LOG_DEBUG, "Float Literal %f", f);
  return ast_literal_float(f);
}

LiteralExpr *parse_string_literal(str s) {
  char *cstring = str_to_cstring(s);
  fe_parser_log(LOG_DEBUG, "String Literal '%s'", cstring);
  free(cstring);
  return ast_literal_string(s);
}

LiteralExpr *parse_boolean_literal(bool b) {
  fe_parser_log(LOG_DEBUG, "Boolean Literal %s", b ? "true" : "false");
  return ast_literal_boolean(b);
}

ExprList *parse_expr_list(Expr *head, ExprList *tail) {
  ExprList* list = ast_expr_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Expr List (len=%u)", list->len);
  return list;
}

Program *parse_program(ExprList *exprs) {
  Program *prog = ast_program(exprs);
  fe_parser_log(LOG_DEBUG, "Program (len=%u)", prog->len);
  fe_set_ast(prog);
  return prog;
}
