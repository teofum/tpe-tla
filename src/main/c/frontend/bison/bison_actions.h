#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include <ast/ast.h>
#include <support/types.h>

Expr *parse_literal_expr(LiteralExpr *literal);
Expr *parse_unary_expr(UnaryExpr *unary);
Expr *parse_binary_expr(BinaryExpr *binary);
Expr *parse_group_expr(GroupExpr *group);

LiteralExpr *parse_integer_literal(i64 i);
LiteralExpr *parse_float_literal(f64 f);
LiteralExpr *parse_string_literal(str s);
LiteralExpr *parse_boolean_literal(bool b);

UnaryExpr *parse_unary(TokenLabel op, Expr *expr);
BinaryExpr *parse_binary(Expr *left, TokenLabel op, Expr *right);
GroupExpr *parse_group(Expr *expr);

ExprList *parse_expr_list(Expr *head, ExprList *tail);
Program *parse_program(ExprList *exprs);

#endif
