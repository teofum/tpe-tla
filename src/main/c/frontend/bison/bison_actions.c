#include <ast/ast.h>
#include <frontend/frontend.h>
#include <support/types.h>
#include <support/util.h>

#include "bison_actions.h"

Expr *parse_literal_expr(LiteralExpr *literal) {
  fe_parser_log(LOG_DEBUG, "Literal Expr");
  return ast_expr_literal(literal);
}

Expr *parse_variable_expr(VariableExpr *var) {
  fe_parser_log(LOG_DEBUG, "Variable Expr");
  return ast_expr_variable(var);
}

Expr *parse_unary_expr(UnaryExpr *unary) {
  fe_parser_log(LOG_DEBUG, "Unary Expr");
  return ast_expr_unary(unary);
}

Expr *parse_binary_expr(BinaryExpr *binary) {
  fe_parser_log(LOG_DEBUG, "Binary Expr");
  return ast_expr_binary(binary);
}

Expr *parse_group_expr(GroupExpr *group) {
  fe_parser_log(LOG_DEBUG, "Group Expr");
  return ast_expr_group(group);
}

// -----------------------------------------------------------------------------

LiteralExpr *parse_integer_literal(IntegerLiteral *i) {
  fe_parser_log(LOG_DEBUG, "Integer Literal %lld", i->value);
  return ast_literal_integer(i);
}

LiteralExpr *parse_float_literal(FloatLiteral *f) {
  fe_parser_log(LOG_DEBUG, "Float Literal %f", f->value);
  return ast_literal_float(f);
}

LiteralExpr *parse_string_literal(StringLiteral *s) {
  fe_parser_log(LOG_DEBUG, "String Literal %s", s->meta->lexeme);
  return ast_literal_string(s);
}

LiteralExpr *parse_boolean_literal(BooleanLiteral *b) {
  fe_parser_log(LOG_DEBUG, "Boolean Literal %s", b->meta->lexeme);
  return ast_literal_boolean(b);
}

// -----------------------------------------------------------------------------

VariableExpr *parse_identifier_variable(TokenMeta *id) {
  fe_parser_log(LOG_DEBUG, "Identifier Variable %s", id->lexeme);
  return ast_variable_identifier(id);
}

VariableExpr *parse_struct_member_variable(Expr *struct_expr, TokenMeta *op, TokenMeta *id) {
  fe_parser_log(LOG_DEBUG, "Struct Member %s", id->lexeme);
  ast_free_meta(op);
  return ast_variable_struct_member(struct_expr, id);
}

// -----------------------------------------------------------------------------

UnaryExpr *parse_unary(TokenMeta *op, Expr *expr) {
  fe_parser_log(LOG_DEBUG, "Unary '%s'", op->lexeme);
  return ast_unary(op, expr);
}

BinaryExpr *parse_binary(Expr *left, TokenMeta *op, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Binary '%s'", op->lexeme);
  return ast_binary(left, op, right);
}

GroupExpr *parse_group(TokenMeta *open, Expr *expr, TokenMeta *close) {
  fe_parser_log(LOG_DEBUG, "Group");
  ast_free_meta(open);
  ast_free_meta(close);
  return ast_group(expr);
}

// -----------------------------------------------------------------------------

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
