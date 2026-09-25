#include <ast/ast.h>
#include <frontend/frontend.h>
#include <support/types.h>
#include <support/util.h>

#include "bison_actions.h"

Stmt *parse_expr_stmt(Expr *expr) {
  fe_parser_log(LOG_DEBUG, "Expression Stmt");
  return ast_stmt_expr(expr);
}

Stmt *parse_declaration_stmt(DeclarationStmt *decl) {
  fe_parser_log(LOG_DEBUG, "Declaration Stmt");
  return ast_stmt_decl(decl);
}

Stmt *parse_type_alias_stmt(TypeAliasStmt *alias) {
  fe_parser_log(LOG_DEBUG, "Type Alias Stmt");
  return ast_stmt_alias(alias);
}

Stmt *parse_error_stmt() {
  fe_parser_log(LOG_DEBUG, "ERROR Stmt");
  return ast_stmt_error();
}

DeclarationStmt *parse_declaration(Identifier *left, Type *type, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Declaration for %s", left->lexeme);
  return ast_declaration(left, type, right);
}

TypeAliasStmt *parse_type_alias(Identifier *left, Type *right) {
  fe_parser_log(LOG_DEBUG, "Type alias for %s", left->lexeme);
  return ast_type_alias(left, right);
}

// -----------------------------------------------------------------------------

#define PARSE_EXPR_FUNC(expr_name, T, ...)            \
Expr *parse_##expr_name##_expr(T *expr) {             \
  fe_parser_log(LOG_DEBUG, __VA_ARGS__);              \
  return ast_expr_##expr_name(expr);                  \
}                                                     \

PARSE_EXPR_FUNC(literal, LiteralExpr, "Literal Expr")
PARSE_EXPR_FUNC(variable, VariableExpr, "Variable Expr")
PARSE_EXPR_FUNC(unary, UnaryExpr, "Unary Expr")
PARSE_EXPR_FUNC(binary, BinaryExpr, "Binary Expr")
PARSE_EXPR_FUNC(group, GroupExpr, "Group Expr")
PARSE_EXPR_FUNC(assignment, AssignmentExpr, "Assignment Expr")
PARSE_EXPR_FUNC(if, IfExpr, "If Expr")
PARSE_EXPR_FUNC(for, ForExpr, "For Expr")
PARSE_EXPR_FUNC(block, BlockExpr, "Block Expr (len=%u)", expr->len)

// -----------------------------------------------------------------------------

LiteralExpr *parse_integer_literal(IntegerLiteral *i) {
  fe_parser_log(LOG_DEBUG, "Integer Literal %lld", i->value);
  return ast_integer_literal(i);
}

LiteralExpr *parse_float_literal(FloatLiteral *f) {
  fe_parser_log(LOG_DEBUG, "Float Literal %f", f->value);
  return ast_float_literal(f);
}

LiteralExpr *parse_string_literal(StringLiteral *s) {
  fe_parser_log(LOG_DEBUG, "String Literal %s", s->meta->lexeme);
  return ast_string_literal(s);
}

LiteralExpr *parse_boolean_literal(BooleanLiteral *b) {
  fe_parser_log(LOG_DEBUG, "Boolean Literal %s", b->meta->lexeme);
  return ast_boolean_literal(b);
}

LiteralExpr *parse_nil_literal() {
  fe_parser_log(LOG_DEBUG, "Nil Literal");
  return ast_nil_literal();
}

LiteralExpr *parse_list_literal(ExprList *exprs) {
  LiteralExpr *literal = ast_list_literal(exprs);
  fe_parser_log(LOG_DEBUG, "List Literal (len=%u)", literal->list->len);
  return literal;
}

LiteralExpr *parse_map_literal(MapEntryList *entries) {
  LiteralExpr *literal = ast_map_literal(entries);
  fe_parser_log(LOG_DEBUG, "Map Literal (len=%u)", literal->map->len);
  return literal;
}

LiteralExpr *parse_struct_literal(StructLiteralFieldList *fields) {
  LiteralExpr *literal = ast_struct_literal(fields);
  fe_parser_log(LOG_DEBUG, "Struct Literal (len=%u)", literal->struct_literal->len);
  return literal;
}

// -----------------------------------------------------------------------------

VariableExpr *parse_named_variable(Identifier *id) {
  fe_parser_log(LOG_DEBUG, "Named Variable %s", id->lexeme);
  return ast_variable_named(id);
}

VariableExpr *parse_struct_member_variable(VariableExpr *struct_expr, Identifier *id) {
  fe_parser_log(LOG_DEBUG, "Struct Member %s", id->lexeme);
  return ast_variable_struct_member(struct_expr, id);
}

VariableExpr *parse_indexed_variable(VariableExpr *container, Expr *index) {
  fe_parser_log(LOG_DEBUG, "Indexed variable");
  return ast_variable_indexed(container, index);
}

// -----------------------------------------------------------------------------

UnaryExpr *parse_unary(TokenLabel op, Expr *expr) {
  fe_parser_log(LOG_DEBUG, "Unary '%s'", TOKEN_LEXEME[op]);
  return ast_unary(op, expr);
}

BinaryExpr *parse_binary(Expr *left, TokenLabel op, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Binary '%s'", TOKEN_LEXEME[op]);
  return ast_binary(left, op, right);
}

GroupExpr *parse_group(Expr *expr) {
  fe_parser_log(LOG_DEBUG, "Group");
  return ast_group(expr);
}

AssignmentExpr *parse_assignment(VariableExpr *left, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Assignment");
  return ast_assignment(left, right);
}

IfExpr *parse_if(Expr *condition, BlockExpr *true_branch, BlockExpr *false_branch) {
  fe_parser_log(LOG_DEBUG, "If");
  Expr *false_expr = false_branch ? ast_expr_block(false_branch) : NULL;
  return ast_if(condition, ast_expr_block(true_branch), false_expr);
}

IfExpr *parse_nested_if(Expr *condition, BlockExpr *true_branch, IfExpr *false_branch) {
  fe_parser_log(LOG_DEBUG, "If");
  return ast_if(condition, ast_expr_block(true_branch), ast_expr_if(false_branch));
}

ForExpr *parse_for(Identifier *var, Identifier *idx, Expr *iterable, BlockExpr *body) {
  fe_parser_log(LOG_DEBUG, "For");
  return ast_for(var, idx, iterable, ast_expr_block(body));
}

BlockExpr *parse_block(StmtList *statements) {
  BlockExpr *block = ast_block(statements);
  fe_parser_log(LOG_DEBUG, "Block (len=%u)", block->len);
  return block;
}

// -----------------------------------------------------------------------------

Type *parse_named_type(Identifier *id) {
  fe_parser_log(LOG_DEBUG, "Named Type %s", id->lexeme);
  return ast_named_type(id);
}

Type *parse_list_type(Type *item) {
  fe_parser_log(LOG_DEBUG, "List Type");
  return ast_list_type(item);
}

Type *parse_map_type(Type *key, Type *value) {
  fe_parser_log(LOG_DEBUG, "Map Type");
  return ast_map_type(key, value);
}

Type *parse_struct_type(StructFieldList *fields) {
  fe_parser_log(LOG_DEBUG, "Struct");
  return ast_struct_type(fields);
}

Type *parse_union_type(TypeList *types) {
  fe_parser_log(LOG_DEBUG, "Union");
  return ast_union_type(types);
}

Type *parse_optional_type(Type *type) {
  fe_parser_log(LOG_DEBUG, "Optional");

  // Optional types don't exist in the AST or backend; it's simply syntax sugar for a {T, nil} union
  TypeList *types = ast_type_list(type, ast_type_list(ast_nil_type(), NULL));
  return ast_union_type(types);
}

Type *parse_tuple_type(TypeList *types) {
  fe_parser_log(LOG_DEBUG, "Tuple");
  return ast_tuple_type(types);
}

Type *parse_enum_type(IdentifierList *values) {
  fe_parser_log(LOG_DEBUG, "Enum");
  return ast_enum_type(values);
}

Type *parse_nil_type() {
  fe_parser_log(LOG_DEBUG, "Nil Type");
  return ast_nil_type();
}

// -----------------------------------------------------------------------------

StructFieldList *parse_struct_field_list(StructField *head, StructFieldList *tail) {
  StructFieldList* list = ast_struct_field_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Struct Field List (len=%u)", list->len);
  return list;
}

StructLiteralFieldList *parse_struct_literal_field_list(StructLiteralField *head, StructLiteralFieldList *tail) {
  StructLiteralFieldList* list = ast_struct_literal_field_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Struct Literal Field List (len=%u)", list->len);
  return list;
}

MapEntryList *parse_map_entry_list(MapEntry *head, MapEntryList *tail) {
  MapEntryList* list = ast_map_entry_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Map Entry List (len=%u)", list->len);
  return list;
}

TypeList *parse_type_list(Type *head, TypeList *tail) {
  TypeList* list = ast_type_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Type List (len=%u)", list->len);
  return list;
}

IdentifierList *parse_identifier_list(Identifier *head, IdentifierList *tail) {
  IdentifierList* list = ast_identifier_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Identifier List (len=%u)", list->len);
  return list;
}

StmtList *parse_stmt_list(Stmt *head, StmtList *tail) {
  StmtList* list = ast_stmt_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Stmt List (len=%u)", list->len);
  return list;
}

ExprList *parse_expr_list(Expr *head, ExprList *tail) {
  ExprList* list = ast_expr_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Expr List (len=%u)", list->len);
  return list;
}

// -----------------------------------------------------------------------------

StructField *parse_struct_field(Identifier *id, Type *type, Expr *default_value) {
  fe_parser_log(LOG_DEBUG, "Struct Field %s", id->lexeme);
  return ast_struct_field(id, type, default_value);
}

StructLiteralField *parse_struct_literal_field(Identifier *id, Expr *value) {
  fe_parser_log(LOG_DEBUG, "Struct Literal Field %s", id->lexeme);
  return ast_struct_literal_field(id, value);
}

MapEntry *parse_map_entry_literal(LiteralExpr *key, Expr *value) {
  fe_parser_log(LOG_DEBUG, "Map Entry");
  return ast_map_entry(ast_expr_literal(key), value);
}

MapEntry *parse_map_entry_block(BlockExpr *key, Expr *value) {
  fe_parser_log(LOG_DEBUG, "Map Entry");
  return ast_map_entry(ast_expr_block(key), value);
}

Program *parse_program(StmtList *statements) {
  Program *prog = ast_program(statements);
  fe_parser_log(LOG_DEBUG, "Program (len=%u)", prog->len);
  fe_set_ast(prog);
  return prog;
}

// -----------------------------------------------------------------------------

void parse_error(Location *loc, const char *message) {
  fe_parser_log(LOG_ERROR, "%s\n\tAt line %u, col %u", message, loc->first_line, loc->first_column);
}
