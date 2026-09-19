#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include <ast/ast.h>
#include <support/types.h>

Expr *parse_literal_expr(LiteralExpr *literal);

LiteralExpr *parse_integer_literal(i64 i);
LiteralExpr *parse_float_literal(f64 f);
LiteralExpr *parse_string_literal(str s);
LiteralExpr *parse_boolean_literal(bool b);

ExprList *parse_expr_list(Expr *head, ExprList *tail);
Program *parse_program(ExprList *exprs);

#endif
