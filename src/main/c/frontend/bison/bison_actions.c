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

DeclarationStmt *parse_declaration(TokenMeta *left, TokenMeta *l_op, Type *type, TokenMeta *r_op, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Declaration for %s", left->lexeme);
  ast_free_meta(l_op);
  ast_free_meta(r_op);
  return ast_declaration(left, type, right);
}

TypeAliasStmt *parse_type_alias(TokenMeta *left, TokenMeta *is, Type *right) {
  fe_parser_log(LOG_DEBUG, "Type alias for %s", left->lexeme);
  ast_free_meta(is);
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

VariableExpr *parse_named_variable(TokenMeta *id) {
  fe_parser_log(LOG_DEBUG, "Named Variable %s", id->lexeme);
  return ast_variable_named(id);
}

VariableExpr *parse_struct_member_variable(VariableExpr *struct_expr, TokenMeta *op, TokenMeta *id) {
  fe_parser_log(LOG_DEBUG, "Struct Member %s", id->lexeme);
  ast_free_meta(op);
  return ast_variable_struct_member(struct_expr, id);
}

VariableExpr *parse_indexed_variable(VariableExpr *container, TokenMeta *open, Expr *index, TokenMeta *close) {
  fe_parser_log(LOG_DEBUG, "Indexed variable");
  ast_free_meta(open);
  ast_free_meta(close);
  return ast_variable_indexed(container, index);
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

AssignmentExpr *parse_assignment(VariableExpr *left, TokenMeta *op, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Assignment");
  ast_free_meta(op);
  return ast_assignment(left, right);
}

IfExpr *parse_if(TokenMeta *if_kw, Expr *condition, Expr *true_branch, TokenMeta *else_kw, Expr *false_branch) {
  fe_parser_log(LOG_DEBUG, "If");
  ast_free_meta(if_kw);
  ast_free_meta(else_kw);
  return ast_if(condition, true_branch, false_branch);
}

ForExpr *parse_for(TokenMeta *for_kw, TokenMeta *var, TokenMeta *comma, TokenMeta *idx, TokenMeta *in, Expr *iterable, Expr *body) {
  fe_parser_log(LOG_DEBUG, "For");
  ast_free_meta(for_kw);
  ast_free_meta(comma);
  ast_free_meta(in);
  return ast_for(var, idx, iterable, body);
}

BlockExpr *parse_block(TokenMeta *open, StmtList *statements, Expr *final, TokenMeta *close) {
  BlockExpr *block = ast_block(statements, final);
  fe_parser_log(LOG_DEBUG, "Block (len=%u)", block->len);
  ast_free_meta(open);
  ast_free_meta(close);
  return block;
}

// -----------------------------------------------------------------------------

Type *parse_named_type(TokenMeta *id) {
  fe_parser_log(LOG_DEBUG, "Named Type %s", id->lexeme);
  return ast_type_named(id);
}

Type *parse_list_type(TokenMeta *open, TokenMeta *close, Type *item) {
  fe_parser_log(LOG_DEBUG, "List Type");
  ast_free_meta(open);
  ast_free_meta(close);
  return ast_type_array(item);
}

Type *parse_map_type(TokenMeta *open, Type *key, TokenMeta *close, Type *value) {
  fe_parser_log(LOG_DEBUG, "Map Type");
  ast_free_meta(open);
  ast_free_meta(close);
  return ast_type_map(key, value);
}

Type *parse_struct_type(TokenMeta *kw, TokenMeta *open, StructFieldList *fields, TokenMeta *close, TokenMeta *trailing) {
  fe_parser_log(LOG_DEBUG, "Struct");
  ast_free_meta(kw);
  ast_free_meta(open);
  ast_free_meta(close);
  ast_free_meta(trailing);
  return ast_type_struct(fields);
}

Type *parse_union_type(TokenMeta *kw, TokenMeta *open, TypeList *types, TokenMeta *close, TokenMeta *trailing) {
  fe_parser_log(LOG_DEBUG, "Union");
  ast_free_meta(kw);
  ast_free_meta(open);
  ast_free_meta(close);
  ast_free_meta(trailing);
  return ast_type_union(types);
}

Type *parse_optional_type(Type *type, TokenMeta *question) {
  fe_parser_log(LOG_DEBUG, "Optional");
  ast_free_meta(question);

  // Optional types don't exist in the AST or backend; it's simply syntax sugar for a {T, nil} union
  TypeList *types = ast_type_list(type, ast_type_list(ast_type_nil(), NULL));
  return ast_type_union(types);
}

Type *parse_tuple_type(TokenMeta *open, TypeList *types, TokenMeta *close, TokenMeta *trailing) {
  fe_parser_log(LOG_DEBUG, "Tuple");
  ast_free_meta(open);
  ast_free_meta(close);
  ast_free_meta(trailing);
  return ast_type_tuple(types);
}

Type *parse_nil_type(TokenMeta *tok) {
  fe_parser_log(LOG_DEBUG, "Nil Type");
  ast_free_meta(tok);
  return ast_type_nil();
}

// -----------------------------------------------------------------------------

StructFieldList *parse_struct_field_list(StructField *head, TokenMeta *comma, StructFieldList *tail) {
  StructFieldList* list = ast_struct_field_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Struct Field List (len=%u)", list->len);
  ast_free_meta(comma);
  return list;
}

TypeList *parse_type_list(Type *head, TokenMeta *comma, TypeList *tail) {
  TypeList* list = ast_type_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Type List (len=%u)", list->len);
  ast_free_meta(comma);
  return list;
}

StmtList *parse_stmt_list(Stmt *head, StmtList *tail) {
  StmtList* list = ast_stmt_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Stmt List (len=%u)", list->len);
  return list;
}

StructField *parse_struct_field(TokenMeta *id, TokenMeta *colon, Type *type, TokenMeta *eq, Expr *default_value) {
  fe_parser_log(LOG_DEBUG, "Struct Field %s", id->lexeme);
  ast_free_meta(colon);
  ast_free_meta(eq);
  return ast_struct_field(id, type, default_value);
}

Program *parse_program(StmtList *statements) {
  Program *prog = ast_program(statements);
  fe_parser_log(LOG_DEBUG, "Program (len=%u)", prog->len);
  fe_set_ast(prog);
  return prog;
}
