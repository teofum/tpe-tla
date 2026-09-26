#include <stdlib.h>
#include <support/types.h>
#include <support/util.h>
#include <support/str.h>

#include "ast.h"

// -----------------------------------------------------------------------------

#define AST_LIST_IMPL(T, name)                                            \
T##List *ast_##name##_list(T *head, T##List *tail) {                      \
  T##List* list = new(T##List);                                           \
  list->head = head;                                                      \
  if (tail) {                                                             \
    list->tail = tail;                                                    \
    list->len = tail->len + 1;                                            \
  } else {                                                                \
    list->tail = NULL;                                                    \
    list->len = 1;                                                        \
  }                                                                       \
  return list;                                                            \
}                                                                         \
void ast_free_##name##_list(T##List *list, bool free_items) {             \
  if (!list) return;                                                      \
  if (free_items) ast_free_##name(list->head);                            \
  ast_free_##name##_list(list->tail, free_items);                         \
  free(list);                                                             \
}                                                                         \
void ast_consume_##name##_list(T##List *list, T ***items, u32 *len) { \
  *items = new_array(T *, list->len);                                     \
  *len = list->len;                                                       \
  T##List *tail = list;                                                   \
  for (u32 i = 0; tail != NULL && i < *len; i++) {                        \
    (*items)[*len - i - 1] = tail->head;                                  \
    tail = tail->tail;                                                    \
  }                                                                       \
  ast_free_##name##_list(list, false);                                    \
}                                                                         \

AST_LIST_IMPL(Stmt, stmt)
AST_LIST_IMPL(Expr, expr)
AST_LIST_IMPL(StructField, struct_field)
AST_LIST_IMPL(StructLiteralField, struct_literal_field)
AST_LIST_IMPL(MapEntry, map_entry)
AST_LIST_IMPL(Type, type)
AST_LIST_IMPL(Identifier, identifier)
AST_LIST_IMPL(Parameter, parameter)
AST_LIST_IMPL(Argument, argument)

// -----------------------------------------------------------------------------

#define AST_STMT_FUNC(f_name, T, stmt_t, p_name)  \
Stmt *f_name(T *p_name) {                         \
  Stmt *stmt = new(Stmt);                         \
  *stmt = (Stmt){                                 \
    .type = stmt_t,                               \
    .p_name = p_name,                             \
  };                                              \
  return stmt;                                    \
}                                                 \

AST_STMT_FUNC(ast_stmt_expr, Expr, STMT_EXPR, expr)
AST_STMT_FUNC(ast_stmt_decl, DeclarationStmt, STMT_DECLARATION, decl)
AST_STMT_FUNC(ast_stmt_alias, TypeAliasStmt, STMT_TYPE_ALIAS, type_alias)
AST_STMT_FUNC(ast_stmt_function_def, FunctionDef, STMT_FUNCTION_DEF, function_def)

Stmt *ast_stmt_error() {
  Stmt *stmt = new(Stmt);
  stmt->type = STMT_PARSE_ERROR;
  return stmt;
}

// -----------------------------------------------------------------------------

DeclarationStmt *ast_declaration(Identifier *id, Type *type, Expr *expr) {
  DeclarationStmt *decl = new(DeclarationStmt);
  *decl = (DeclarationStmt){
    .left = id,
    .right = expr,
    .type = type,
  };

  return decl;
}

TypeAliasStmt *ast_type_alias(Identifier *id, Type *type) {
  TypeAliasStmt *alias = new(TypeAliasStmt);
  *alias = (TypeAliasStmt){
    .alias = id,
    .type = type,
  };

  return alias;
}

// -----------------------------------------------------------------------------

#define AST_EXPR_FUNC(f_name, T, expr_t, p_name)  \
Expr *f_name(T *p_name) {                         \
  Expr *expr = new(Expr);                         \
  *expr = (Expr){                                 \
    .type = expr_t,                               \
    .p_name = p_name,                             \
  };                                              \
  return expr;                                    \
}                                                 \

AST_EXPR_FUNC(ast_expr_literal, LiteralExpr, EXPR_LITERAL, literal)
AST_EXPR_FUNC(ast_expr_variable, VariableExpr, EXPR_VARIABLE, variable)
AST_EXPR_FUNC(ast_expr_unary, UnaryExpr, EXPR_UNARY, unary)
AST_EXPR_FUNC(ast_expr_binary, BinaryExpr, EXPR_BINARY, binary)
AST_EXPR_FUNC(ast_expr_group, GroupExpr, EXPR_GROUP, group)
AST_EXPR_FUNC(ast_expr_assignment, AssignmentExpr, EXPR_ASSIGNMENT, assignment)
AST_EXPR_FUNC(ast_expr_if, IfExpr, EXPR_IF, if_expr)
AST_EXPR_FUNC(ast_expr_for, ForExpr, EXPR_FOR, for_expr)
AST_EXPR_FUNC(ast_expr_block, BlockExpr, EXPR_BLOCK, block)
AST_EXPR_FUNC(ast_expr_function_call, FunctionCallExpr, EXPR_FUNCTION_CALL, function_call)

// -----------------------------------------------------------------------------

#define AST_LITERAL_FUNC(f_name, T, literal_t, p_name)  \
LiteralExpr *f_name(T *p_name) {                        \
  LiteralExpr *literal = new(LiteralExpr);              \
  *literal = (LiteralExpr){                             \
    .type = literal_t,                                  \
    .p_name = p_name,                                   \
  };                                                    \
  return literal;                                       \
}                                                       \

AST_LITERAL_FUNC(ast_integer_literal, IntegerLiteral, L_INTEGER, integer)
AST_LITERAL_FUNC(ast_float_literal, FloatLiteral, L_FLOAT, floating)
AST_LITERAL_FUNC(ast_string_literal, StringLiteral, L_STRING, string)
AST_LITERAL_FUNC(ast_boolean_literal, BooleanLiteral, L_BOOL, boolean)

LiteralExpr *ast_nil_literal() {
  LiteralExpr *literal = new(LiteralExpr);
  literal->type = L_NIL;
  return literal;
}

LiteralExpr *ast_list_literal(ExprList *exprs) {
  ListLiteral *list = new(ListLiteral);
  if (exprs) ast_consume_expr_list(exprs, &list->exprs, &list->len);

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){ .type = L_LIST, .list = list };
  return literal;
}

LiteralExpr *ast_short_list_literal(Expr *value, Expr *count) {
  ShortListLiteral *list = new(ShortListLiteral);
  *list = (ShortListLiteral){ .value = value, .count = count };

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){ .type = L_LIST_SHORT, .short_list = list };
  return literal;
}

LiteralExpr *ast_map_literal(MapEntryList *entries) {
  MapLiteral *map = new(MapLiteral);
  ast_consume_map_entry_list(entries, &map->entries, &map->len);

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){ .type = L_MAP, .map = map };
  return literal;
}

LiteralExpr *ast_struct_literal(StructLiteralFieldList *fields) {
  StructLiteral *struct_literal = new(StructLiteral);
  ast_consume_struct_literal_field_list(fields, &struct_literal->fields, &struct_literal->len);

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){ .type = L_STRUCT, .struct_literal = struct_literal };
  return literal;
}

LiteralExpr *ast_enum_literal(Type *type, Identifier *id) {
  EnumLiteral *enum_literal = new(EnumLiteral);
  *enum_literal = (EnumLiteral){ .type = type, .value = id };

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){ .type = L_ENUM, .enum_literal = enum_literal };
  return literal;
}

LiteralExpr *ast_range_literal(Expr *start, Expr *end, bool inclusive) {
  RangeLiteral *range = new(RangeLiteral);
  *range = (RangeLiteral){ .start = start, .end = end, .inclusive = inclusive };

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){ .type = L_RANGE, .range = range };
  return literal;
}

// -----------------------------------------------------------------------------

VariableExpr *ast_variable_named(Identifier *id) {
  NamedVariable *named_var = new(NamedVariable);
  *named_var = (NamedVariable){ .name = id };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .type = V_NAMED,
    .named = named_var,
  };

  return var;
}

VariableExpr *ast_variable_struct_member(VariableExpr *struct_expr, Identifier *id) {
  StructMemberVariable *struct_var = new(StructMemberVariable);
  *struct_var = (StructMemberVariable){
    .struct_expr = struct_expr,
    .name = id,
  };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .type = V_STRUCT_MEMBER,
    .struct_member = struct_var,
  };

  return var;
}

VariableExpr *ast_variable_indexed(VariableExpr *container, Expr *index) {
  IndexedVariable *idx_var = new(IndexedVariable);
  *idx_var = (IndexedVariable){
    .container = container,
    .index = index,
  };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .type = V_INDEX,
    .indexed = idx_var,
  };

  return var;
}

// -----------------------------------------------------------------------------

UnaryExpr *ast_unary(TokenLabel op, Expr *expr) {
  UnaryExpr *unary = new(UnaryExpr);
  *unary = (UnaryExpr){
    .op = op,
    .expr = expr,
  };

  return unary;
}

BinaryExpr *ast_binary(Expr *left, TokenLabel op, Expr *right) {
  BinaryExpr *binary = new(BinaryExpr);
  *binary = (BinaryExpr){
    .left = left,
    .op = op,
    .right = right,
  };

  return binary;
}

GroupExpr *ast_group(Expr *expr) {
  GroupExpr *group = new(GroupExpr);
  *group = (GroupExpr){ .inner_expr = expr };

  return group;
}

AssignmentExpr *ast_assignment(VariableExpr *left, Expr *right) {
  AssignmentExpr *assign = new(AssignmentExpr);
  *assign = (AssignmentExpr){
    .left = left,
    .right = right,
  };

  return assign;
}

IfExpr *ast_if(Expr *condition, Expr *true_branch, Expr *false_branch) {
  IfExpr *if_expr = new(IfExpr);
  *if_expr = (IfExpr){
    .condition = condition,
    .true_branch = true_branch,
    .false_branch = false_branch,
  };

  return if_expr;
}

ForExpr *ast_for(Identifier *var, Identifier *idx, Expr *iterable, Expr *body) {
  ForExpr *for_expr = new(ForExpr);
  *for_expr = (ForExpr){
    .var_id = var,
    .idx_id = idx,
    .iterable = iterable,
    .body = body,
  };

  return for_expr;
}

BlockExpr *ast_block(StmtList *statements) {
  BlockExpr* block = new(BlockExpr);
  ast_consume_stmt_list(statements, &block->statements, &block->len);
  return block;
}

FunctionCallExpr *ast_function_call(Identifier *id, ArgumentList *args, Expr *composable) {
  FunctionCallExpr *function_call = new(FunctionCallExpr);
  *function_call = (FunctionCallExpr){
    .name = id,
    .composable = composable,
  };
  ast_consume_argument_list(args, &function_call->args, &function_call->args_len);

  return function_call;
}

// -----------------------------------------------------------------------------

Type *ast_named_type(Identifier *id) {
  NamedType *named_type = new(NamedType);
  *named_type = (NamedType){ .name = id };

  Type *type = new(Type);
  *type = (Type){ .type = T_NAMED, .named = named_type };

  return type;
}

Type *ast_list_type(Type *item_type) {
  ListType *list = new(ListType);
  *list = (ListType){ .item_type = item_type };

  Type *type = new(Type);
  *type = (Type){ .type = T_LIST, .list = list };

  return type;
}

Type *ast_map_type(Type *key_type, Type *value_type) {
  MapType *map = new(MapType);
  *map = (MapType){
    .key_type = key_type,
    .value_type = value_type,
  };

  Type *type = new(Type);
  *type = (Type){ .type = T_MAP, .map = map };

  return type;
}

Type *ast_struct_type(StructFieldList *fields) {
  StructType *struct_type = new(StructType);
  ast_consume_struct_field_list(fields, &struct_type->fields, &struct_type->len);

  Type *type = new(Type);
  *type = (Type){ .type = T_STRUCT, .struct_type = struct_type };

  return type;
}

Type *ast_union_type(TypeList *types) {
  UnionType *union_type = new(UnionType);
  ast_consume_type_list(types, &union_type->types, &union_type->len);

  Type *type = new(Type);
  *type = (Type){ .type = T_UNION, .union_type = union_type };

  return type;
}

Type *ast_tuple_type(TypeList *types) {
  TupleType *tuple = new(TupleType);
  ast_consume_type_list(types, &tuple->types, &tuple->len);

  Type *type = new(Type);
  *type = (Type){ .type = T_TUPLE, .tuple = tuple };

  return type;
}

Type *ast_short_tuple_type(Type *inner_type, Expr *count) {
  ShortTupleType *tuple = new(ShortTupleType);
  *tuple = (ShortTupleType){ .type = inner_type, .count = count };

  Type *type = new(Type);
  *type = (Type){ .type = T_TUPLE_SHORT, .short_tuple = tuple };

  return type;
}

Type *ast_enum_type(IdentifierList *values) {
  EnumType *enum_type = new(EnumType);
  ast_consume_identifier_list(values, &enum_type->values, &enum_type->len);

  Type *type = new(Type);
  *type = (Type){ .type = T_ENUM, .enum_type = enum_type };

  return type;
}

Type *ast_nil_type() {
  Type *type = new(Type);
  *type = (Type){ .type = T_NIL };

  return type;
}

// -----------------------------------------------------------------------------

FunctionDef *ast_function_def(Identifier *id, ParameterList *params, Type *return_type, BlockExpr *body) {
  FunctionDef *function = new(FunctionDef);
  *function = (FunctionDef){
    .name = id,
    .return_type = return_type,
    .body = body,
  };
  if (params) {
    ast_consume_parameter_list(params, &function->params, &function->params_len);
  }

  return function;
}

// -----------------------------------------------------------------------------

StructField *ast_struct_field(Identifier *id, Type *type, Expr *default_value) {
  StructField *field = new(StructField);
  *field = (StructField){
    .name = id,
    .type = type,
    .default_value = default_value,
  };

  return field;
}

StructLiteralField *ast_struct_literal_field(Identifier *id, Expr *value) {
  StructLiteralField *field = new(StructLiteralField);
  *field = (StructLiteralField){
    .name = id,
    .value = value,
  };

  return field;
}

MapEntry *ast_map_entry(Expr *key, Expr *value) {
  MapEntry *entry = new(MapEntry);
  *entry = (MapEntry){
    .key = key,
    .value = value,
  };

  return entry;
}

Parameter *ast_parameter(Identifier *id, Type *type, Expr *default_value) {
  Parameter *param = new(Parameter);
  *param = (Parameter){
    .name = id,
    .type = type,
    .default_value = default_value,
  };

  return param;
}

Argument *ast_argument(Identifier *id, Expr *value) {
  Argument *arg = new(Argument);
  *arg = (Argument){
    .name = id,
    .value = value,
  };

  return arg;
}

// -----------------------------------------------------------------------------

Program *ast_program(StmtList *statements) {
  Program *prog = new(Program);
  ast_consume_stmt_list(statements, &prog->statements, &prog->len);
  return prog;
}

// -----------------------------------------------------------------------------

void ast_free_stmt(Stmt *stmt) {
  if (!stmt) return;

  switch (stmt->type) {
    case STMT_EXPR: ast_free_expr(stmt->expr); break;
    case STMT_DECLARATION: ast_free_decl(stmt->decl); break;
    case STMT_TYPE_ALIAS: ast_free_alias(stmt->type_alias); break;
    case STMT_FUNCTION_DEF: ast_free_function_def(stmt->function_def); break;
    case STMT_PARSE_ERROR: break;
  }

  free(stmt);
}

void ast_free_decl(DeclarationStmt *decl) {
  if (!decl) return;

  ast_free_identifier(decl->left);
  ast_free_expr(decl->right);
  ast_free_type(decl->type);
  free(decl);
}

void ast_free_alias(TypeAliasStmt *alias) {
  if (!alias) return;

  ast_free_identifier(alias->alias);
  ast_free_type(alias->type);
  free(alias);
}

void ast_free_expr(Expr *expr) {
  if (!expr) return;

  switch (expr->type) {
    case EXPR_LITERAL: ast_free_literal(expr->literal); break;
    case EXPR_VARIABLE: ast_free_variable(expr->variable); break;
    case EXPR_UNARY: ast_free_unary(expr->unary); break;
    case EXPR_BINARY: ast_free_binary(expr->binary); break;
    case EXPR_GROUP: ast_free_group(expr->group); break;
    case EXPR_ASSIGNMENT: ast_free_assignment(expr->assignment); break;
    case EXPR_IF: ast_free_if(expr->if_expr); break;
    case EXPR_FOR: ast_free_for(expr->for_expr); break;
    case EXPR_BLOCK: ast_free_block(expr->block); break;
    case EXPR_FUNCTION_CALL: ast_free_function_call(expr->function_call); break;
  }

  free(expr);
}

void ast_free_literal(LiteralExpr *literal) {
  if (!literal) return;

  switch (literal->type) {
    case L_INTEGER: ast_free_int_literal(literal->integer); break;
    case L_FLOAT: ast_free_float_literal(literal->floating); break;
    case L_STRING: ast_free_string_literal(literal->string); break;
    case L_BOOL: ast_free_bool_literal(literal->boolean); break;
    case L_LIST: ast_free_list_literal(literal->list); break;
    case L_LIST_SHORT: ast_free_short_list_literal(literal->short_list); break;
    case L_MAP: ast_free_map_literal(literal->map); break;
    case L_STRUCT: ast_free_struct_literal(literal->struct_literal); break;
    case L_ENUM: ast_free_enum_literal(literal->enum_literal); break;
    case L_RANGE: ast_free_range_literal(literal->range); break;
    case L_NIL: break;
  }
  free(literal);
}

void ast_free_variable(VariableExpr *var) {
  if (!var) return;

  switch (var->type) {
    case V_NAMED: ast_free_named_variable(var->named); break;
    case V_STRUCT_MEMBER: ast_free_struct_member_variable(var->struct_member); break;
    case V_INDEX: ast_free_indexed_variable(var->indexed); break;
  }
  free(var);
}

void ast_free_unary(UnaryExpr *unary) {
  if (!unary) return;

  ast_free_expr(unary->expr);
  free(unary);
}

void ast_free_binary(BinaryExpr *binary) {
  if (!binary) return;

  ast_free_expr(binary->left);
  ast_free_expr(binary->right);
  free(binary);
}

void ast_free_group(GroupExpr *group) {
  if (!group) return;

  ast_free_expr(group->inner_expr);
  free(group);
}

void ast_free_assignment(AssignmentExpr *assign) {
  if (!assign) return;

  ast_free_variable(assign->left);
  ast_free_expr(assign->right);
  free(assign);
}

void ast_free_if(IfExpr *if_expr) {
  if (!if_expr) return;

  ast_free_expr(if_expr->condition);
  ast_free_expr(if_expr->true_branch);
  ast_free_expr(if_expr->false_branch);
  free(if_expr);
}

void ast_free_for(ForExpr *for_expr) {
  if (!for_expr) return;

  ast_free_identifier(for_expr->var_id);
  ast_free_identifier(for_expr->idx_id);
  ast_free_expr(for_expr->body);
  ast_free_expr(for_expr->iterable);
  free(for_expr);
}

void ast_free_block(BlockExpr *block) {
  if (!block) return;

  for (u32 i = 0; i < block->len; i++) {
    ast_free_stmt(block->statements[i]);
  }
  free(block->statements);
  free(block);
}

void ast_free_function_call(FunctionCallExpr *function_call) {
  if (!function_call) return;

  ast_free_identifier(function_call->name);
  ast_free_expr(function_call->composable);
  for (u32 i = 0; i < function_call->args_len; i++) {
    ast_free_argument(function_call->args[i]);
  }
  if (function_call->args) free(function_call->args);
  free(function_call);
}

void ast_free_int_literal(IntegerLiteral *l) {
  if (!l) return;

  ast_free_token(l->meta);
  free(l);
}

void ast_free_float_literal(FloatLiteral *l) {
  if (!l) return;

  ast_free_token(l->meta);
  free(l);
}

void ast_free_string_literal(StringLiteral *l) {
  if (!l) return;

  str_free(l->value);
  ast_free_token(l->meta);
  free(l);
}

void ast_free_bool_literal(BooleanLiteral *l) {
  if (!l) return;

  ast_free_token(l->meta);
  free(l);
}

void ast_free_list_literal(ListLiteral *l) {
  if (!l) return;

  for (u32 i = 0; i < l->len; i++) {
    ast_free_expr(l->exprs[i]);
  }
  if (l->exprs) free(l->exprs);
  free(l);
}

void ast_free_short_list_literal(ShortListLiteral *l) {
  if (!l) return;

  ast_free_expr(l->value);
  ast_free_expr(l->count);
  free(l);
}

void ast_free_map_literal(MapLiteral *l) {
  if (!l) return;

  for (u32 i = 0; i < l->len; i++) {
    ast_free_map_entry(l->entries[i]);
  }
  free(l->entries);
  free(l);
}

void ast_free_struct_literal(StructLiteral *l) {
  if (!l) return;

  for (u32 i = 0; i < l->len; i++) {
    ast_free_struct_literal_field(l->fields[i]);
  }
  free(l->fields);
  free(l);
}

void ast_free_enum_literal(EnumLiteral *l) {
  if (!l) return;

  ast_free_type(l->type);
  ast_free_identifier(l->value);
  free(l);
}

void ast_free_range_literal(RangeLiteral *l) {
  if (!l) return;

  ast_free_expr(l->start);
  ast_free_expr(l->end);
  free(l);
}

void ast_free_named_variable(NamedVariable *v) {
  if (!v) return;

  ast_free_identifier(v->name);
  free(v);
}

void ast_free_struct_member_variable(StructMemberVariable *v) {
  if (!v) return;

  ast_free_variable(v->struct_expr);
  ast_free_identifier(v->name);
  free(v);
}

void ast_free_indexed_variable(IndexedVariable *v) {
  if (!v) return;

  ast_free_variable(v->container);
  ast_free_expr(v->index);
  free(v);
}

void ast_free_type(Type *t) {
  if (!t) return;

  switch (t->type) {
    case T_NAMED: ast_free_named_type(t->named); break;
    case T_LIST: ast_free_list_type(t->list); break;
    case T_MAP: ast_free_map_type(t->map); break;
    case T_STRUCT: ast_free_struct_type(t->struct_type); break;
    case T_UNION: ast_free_union_type(t->union_type); break;
    case T_TUPLE: ast_free_tuple_type(t->tuple); break;
    case T_TUPLE_SHORT: ast_free_short_tuple_type(t->short_tuple); break;
    case T_ENUM: ast_free_enum_type(t->enum_type); break;
    case T_NIL: break;
  }
  free(t);
}

void ast_free_named_type(NamedType *t) {
  if (!t) return;

  ast_free_identifier(t->name);
  free(t);
}

void ast_free_list_type(ListType *t) {
  if (!t) return;

  ast_free_type(t->item_type);
  free(t);
}

void ast_free_map_type(MapType *t) {
  if (!t) return;

  ast_free_type(t->key_type);
  ast_free_type(t->value_type);
  free(t);
}

void ast_free_struct_type(StructType *t) {
  if (!t) return;

  for (u32 i = 0; i < t->len; i++) {
    ast_free_struct_field(t->fields[i]);
  }
  free(t->fields);
  free(t);
}

void ast_free_union_type(UnionType *t) {
  if (!t) return;

  for (u32 i = 0; i < t->len; i++) {
    ast_free_type(t->types[i]);
  }
  free(t->types);
  free(t);
}

void ast_free_tuple_type(TupleType *t) {
  if (!t) return;

  for (u32 i = 0; i < t->len; i++) {
    ast_free_type(t->types[i]);
  }
  free(t->types);
  free(t);
}

void ast_free_short_tuple_type(ShortTupleType *t) {
  if (!t) return;

  ast_free_type(t->type);
  ast_free_expr(t->count);
  free(t);
}

void ast_free_enum_type(EnumType *t) {
  if (!t) return;

  for (u32 i = 0; i < t->len; i++) {
    ast_free_identifier(t->values[i]);
  }
  free(t->values);
  free(t);
}

void ast_free_function_def(FunctionDef *f) {
  if (!f) return;

  ast_free_identifier(f->name);
  for (u32 i = 0; i < f->params_len; i++) {
    ast_free_parameter(f->params[i]);
  }
  ast_free_type(f->return_type);
  ast_free_block(f->body);
  free(f->params);
  free(f);
}

void ast_free_struct_field(StructField *f) {
  if (!f) return;

  ast_free_identifier(f->name);
  ast_free_type(f->type);
  ast_free_expr(f->default_value);
  free(f);
}

void ast_free_struct_literal_field(StructLiteralField *f) {
  if (!f) return;

  ast_free_identifier(f->name);
  ast_free_expr(f->value);
  free(f);
}

void ast_free_map_entry(MapEntry *e) {
  if (!e) return;

  ast_free_expr(e->key);
  ast_free_expr(e->value);
  free(e);
}

void ast_free_parameter(Parameter *p) {
  if (!p) return;

  ast_free_identifier(p->name);
  ast_free_type(p->type);
  ast_free_expr(p->default_value);
  free(p);
}

void ast_free_argument(Argument *a) {
  if (!a) return;

  ast_free_identifier(a->name);
  ast_free_expr(a->value);
  free(a);
}

void ast_free_identifier(Identifier *id) {
  ast_free_token(id);
}

void ast_free_token(TokenMeta *meta) {
  if (!meta) return;

  free(meta->lexeme);
  free(meta);
}

void ast_free_program(Program *program) {
  if (!program) return;

  for (u32 i = 0; i < program->len; i++) {
    ast_free_stmt(program->statements[i]);
  }
  free(program->statements);
  free(program);
}
