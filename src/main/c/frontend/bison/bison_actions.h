#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include <ast/ast.h>
#include <support/types.h>

Expr *parse_literal_expr(LiteralExpr *literal);
Expr *parse_variable_expr(VariableExpr *var);
Expr *parse_unary_expr(UnaryExpr *unary);
Expr *parse_binary_expr(BinaryExpr *binary);
Expr *parse_group_expr(GroupExpr *group);
Expr *parse_assignment_expr(AssignmentExpr *assign);
Expr *parse_declaration_expr(DeclarationExpr *decl);
Expr *parse_block_expr(BlockExpr *exprs);

LiteralExpr *parse_integer_literal(IntegerLiteral *i);
LiteralExpr *parse_float_literal(FloatLiteral *f);
LiteralExpr *parse_string_literal(StringLiteral *s);
LiteralExpr *parse_boolean_literal(BooleanLiteral *b);

VariableExpr *parse_named_variable(TokenMeta *id);
VariableExpr *parse_struct_member_variable(VariableExpr *struct_expr, TokenMeta *op, TokenMeta *id);
VariableExpr *parse_indexed_variable(VariableExpr *container, TokenMeta *open, Expr *index, TokenMeta *close);

UnaryExpr *parse_unary(TokenMeta *op, Expr *expr);
BinaryExpr *parse_binary(Expr *left, TokenMeta *op, Expr *right);
GroupExpr *parse_group(TokenMeta *open, Expr *expr, TokenMeta *close);
AssignmentExpr *parse_assignment(VariableExpr *left, TokenMeta *op, Expr *right);
DeclarationExpr *parse_declaration(TokenMeta *left, TokenMeta *l_op, Type *type, TokenMeta *r_op, Expr *right);
BlockExpr *parse_block(TokenMeta *open, ExprList *exprs, TokenMeta *close);

Type *parse_named_type(TokenMeta *id);

ExprList *parse_expr_list(Expr *head, ExprList *tail);
Program *parse_program(ExprList *exprs);

#endif
