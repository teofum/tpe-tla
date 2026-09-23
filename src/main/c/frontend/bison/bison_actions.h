#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include <ast/ast.h>
#include <support/types.h>

Stmt *parse_expr_stmt(Expr *expr);
Stmt *parse_declaration_stmt(DeclarationStmt *decl);
Stmt *parse_type_alias_stmt(TypeAliasStmt *alias);

DeclarationStmt *parse_declaration(Identifier *left, TokenMeta *l_op, Type *type, TokenMeta *r_op, Expr *right);
TypeAliasStmt *parse_type_alias(Identifier *left, TokenMeta *is, Type *right);

Expr *parse_literal_expr(LiteralExpr *literal);
Expr *parse_variable_expr(VariableExpr *var);
Expr *parse_unary_expr(UnaryExpr *unary);
Expr *parse_binary_expr(BinaryExpr *binary);
Expr *parse_group_expr(GroupExpr *group);
Expr *parse_assignment_expr(AssignmentExpr *assign);
Expr *parse_if_expr(IfExpr *if_expr);
Expr *parse_for_expr(ForExpr *for_expr);
Expr *parse_block_expr(BlockExpr *block);

LiteralExpr *parse_integer_literal(IntegerLiteral *i);
LiteralExpr *parse_float_literal(FloatLiteral *f);
LiteralExpr *parse_string_literal(StringLiteral *s);
LiteralExpr *parse_boolean_literal(BooleanLiteral *b);
LiteralExpr *parse_nil_literal(TokenMeta *tok);

VariableExpr *parse_named_variable(Identifier *id);
VariableExpr *parse_struct_member_variable(VariableExpr *struct_expr, TokenMeta *op, Identifier *id);
VariableExpr *parse_indexed_variable(VariableExpr *container, TokenMeta *open, Expr *index, TokenMeta *close);

UnaryExpr *parse_unary(TokenMeta *op, Expr *expr);
BinaryExpr *parse_binary(Expr *left, TokenMeta *op, Expr *right);
GroupExpr *parse_group(TokenMeta *open, Expr *expr, TokenMeta *close);
AssignmentExpr *parse_assignment(VariableExpr *left, TokenMeta *op, Expr *right);
IfExpr *parse_if(TokenMeta *if_kw, Expr *condition, Expr *true_branch, TokenMeta *else_kw, Expr *false_branch);
ForExpr *parse_for(TokenMeta *for_kw, Identifier *var, TokenMeta *comma, Identifier *idx, TokenMeta *in, Expr *iterable, Expr *body);
BlockExpr *parse_block(TokenMeta *open, StmtList *statements, Expr *final, TokenMeta *close);

Type *parse_named_type(Identifier *id);
Type *parse_list_type(TokenMeta *open, TokenMeta *close, Type *item);
Type *parse_map_type(TokenMeta *open, Type *key, TokenMeta *close, Type *value);
Type *parse_struct_type(TokenMeta *kw, TokenMeta *open, StructFieldList *fields, TokenMeta *close, TokenMeta *trailing);
Type *parse_union_type(TokenMeta *kw, TokenMeta *open, TypeList *types, TokenMeta *close, TokenMeta *trailing);
Type *parse_optional_type(Type *type, TokenMeta *question);
Type *parse_tuple_type(TokenMeta *open, TypeList *types, TokenMeta *close, TokenMeta *trailing);
Type *parse_enum_type(TokenMeta *kw, TokenMeta *open, IdentifierList *values, TokenMeta *close, TokenMeta *trailing);
Type *parse_nil_type(TokenMeta *tok);

StructFieldList *parse_struct_field_list(StructField *head, TokenMeta *comma, StructFieldList *tail);
TypeList *parse_type_list(Type *head, TokenMeta *comma, TypeList *tail);
IdentifierList *parse_identifier_list(Identifier *head, TokenMeta *comma, IdentifierList *tail);
StmtList *parse_stmt_list(Stmt *head, StmtList *tail);

StructField *parse_struct_field(Identifier *id, TokenMeta *colon, Type *type, TokenMeta *eq, Expr *default_value);
Program *parse_program(StmtList *statements);

#endif
