#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include <ast/ast.h>
#include <support/types.h>

Stmt *parse_expr_stmt(Expr *expr);
Stmt *parse_declaration_stmt(DeclarationStmt *decl);
Stmt *parse_type_alias_stmt(TypeAliasStmt *alias);
Stmt *parse_error_stmt();

DeclarationStmt *parse_declaration(Identifier *left, Type *type, Expr *right);
TypeAliasStmt *parse_type_alias(Identifier *left, Type *right);

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
LiteralExpr *parse_nil_literal();
LiteralExpr *parse_list_literal(ExprList *exprs);
LiteralExpr *parse_map_literal(MapEntryList *entries);
LiteralExpr *parse_struct_literal(StructLiteralFieldList *fields);

VariableExpr *parse_named_variable(Identifier *id);
VariableExpr *parse_struct_member_variable(VariableExpr *struct_expr, Identifier *id);
VariableExpr *parse_indexed_variable(VariableExpr *container, Expr *index);

UnaryExpr *parse_unary(TokenLabel op, Expr *expr);
BinaryExpr *parse_binary(Expr *left, TokenLabel op, Expr *right);
GroupExpr *parse_group(Expr *expr);
AssignmentExpr *parse_assignment(VariableExpr *left, Expr *right);
IfExpr *parse_if(Expr *condition, BlockExpr *true_branch, BlockExpr *false_branch);
IfExpr *parse_nested_if(Expr *condition, BlockExpr *true_branch, IfExpr *false_branch);
ForExpr *parse_for(Identifier *var, Identifier *idx, Expr *iterable, BlockExpr *body);
BlockExpr *parse_block(StmtList *statements);

Type *parse_named_type(Identifier *id);
Type *parse_list_type(Type *item);
Type *parse_map_type(Type *key, Type *value);
Type *parse_struct_type(StructFieldList *fields);
Type *parse_union_type(TypeList *types);
Type *parse_optional_type(Type *type);
Type *parse_tuple_type(TypeList *types);
Type *parse_enum_type(IdentifierList *values);
Type *parse_nil_type();

StructFieldList *parse_struct_field_list(StructField *head, StructFieldList *tail);
StructLiteralFieldList *parse_struct_literal_field_list(StructLiteralField *head, StructLiteralFieldList *tail);
MapEntryList *parse_map_entry_list(MapEntry *head, MapEntryList *tail);
TypeList *parse_type_list(Type *head, TypeList *tail);
IdentifierList *parse_identifier_list(Identifier *head, IdentifierList *tail);
StmtList *parse_stmt_list(Stmt *head, StmtList *tail);
ExprList *parse_expr_list(Expr *head, ExprList *tail);

StructField *parse_struct_field(Identifier *id, Type *type, Expr *default_value);
StructLiteralField *parse_struct_literal_field(Identifier *id, Expr *value);
MapEntry *parse_map_entry_literal(LiteralExpr *key, Expr *value);
MapEntry *parse_map_entry_block(BlockExpr *key, Expr *value);
Program *parse_program(StmtList *statements);

void parse_error(Location *loc, const char *message);

#endif
