#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <error/error.h>
#include <support/hash_map.h>
#include <support/types.h>
#include <support/util.h>
#include <support/state.h>
#include <support/str.h>

#include "ast.h"

const char *import_type_str[] = {
  [IMPORT_URL] = "url",
  [IMPORT_RAW] = "raw",
  [IMPORT_CSV] = "csv",
};

const char *builtin_type_str[] = {
  [T_VOID] = NULL,
  [T_NIL] = "nil",
  [T_INT] = "int",
  [T_FLOAT] = "float",
  [T_STRING] = "string",
  [T_BOOL] = "bool",
  [T_ELEMENT] = "element",
};

// -----------------------------------------------------------------------------

static Location *_next_loc = NULL;
static u64 _next_id = 0;

void ast_set_next_location(Location *loc) {
  _next_loc = loc;
}

static ASTNodeMetadata _ast_get_metadata() {
  return (ASTNodeMetadata){
    .id = _next_id++,
    .loc = *_next_loc,
  };
}

// -----------------------------------------------------------------------------

#define AST_LIST_IMPL(T, name)                                            \
T##List *ast_##name##_list(T *head, T##List *tail) {                      \
  T##List* list = new(T##List);                                           \
  list->meta = _ast_get_metadata();                                       \
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
AST_LIST_IMPL(CSVString, csv_string)
AST_LIST_IMPL(CSVRecord, csv_record)

// -----------------------------------------------------------------------------

#define AST_STMT_FUNC(f_name, T, stmt_t, p_name)  \
Stmt *f_name(T *p_name) {                         \
  Stmt *stmt = new(Stmt);                         \
  *stmt = (Stmt){                                 \
    .meta = _ast_get_metadata(),                  \
    .type = stmt_t,                               \
    .p_name = p_name,                             \
  };                                              \
  return stmt;                                    \
}                                                 \

AST_STMT_FUNC(ast_stmt_expr, Expr, STMT_EXPR, expr)
AST_STMT_FUNC(ast_stmt_alias, TypeAliasStmt, STMT_TYPE_ALIAS, type_alias)
AST_STMT_FUNC(ast_stmt_function_def, FunctionDef, STMT_FUNCTION_DEF, function_def)
AST_STMT_FUNC(ast_stmt_constant, ConstantStmt, STMT_CONSTANT, constant)

Stmt *ast_stmt_export(IdentifierList *ids) {
  Stmt *stmt = new(Stmt);
  *stmt = (Stmt) {
    .meta = _ast_get_metadata(),
    .type = STMT_EXPORT,
    .export = new(ExportDirective),
  };
  stmt->export->meta = _ast_get_metadata();
  ast_consume_identifier_list(ids, &stmt->export->ids, &stmt->export->len);
  return stmt;
}

Stmt *ast_stmt_import(IdentifierList *ids, u32 file_idx) {
  Stmt *stmt = new(Stmt);
  *stmt = (Stmt) {
    .meta = _ast_get_metadata(),
    .type = STMT_IMPORT,
    .import = new(ImportDirective),
  };
  stmt->import->meta = _ast_get_metadata();
  stmt->import->file_idx = file_idx;
  ast_consume_identifier_list(ids, &stmt->import->ids, &stmt->import->len);
  return stmt;
}

Stmt *ast_stmt_decl(PartialDeclarationStmt *pdecl) {
  DeclarationStmt *decl = new(DeclarationStmt);
  *decl = (DeclarationStmt){
    .type = pdecl->type,
    .right = pdecl->right,
    .meta = _ast_get_metadata()
  };
  ast_consume_identifier_list(pdecl->left, &decl->left, &decl->left_count);
  free(pdecl);

  Stmt *stmt = new(Stmt);
  *stmt = (Stmt){
    .meta = _ast_get_metadata(),
    .type = STMT_DECLARATION,
    .decl = decl,
  };
  return stmt;
}

Stmt *ast_stmt_error() {
  Stmt *stmt = new(Stmt);
  stmt->meta = _ast_get_metadata();
  stmt->type = STMT_PARSE_ERROR;
  return stmt;
}

// -----------------------------------------------------------------------------

PartialDeclarationStmt *ast_declaration(IdentifierList *ids, Type *type, Expr *expr) {
  PartialDeclarationStmt *decl = new(PartialDeclarationStmt);
  *decl = (PartialDeclarationStmt){
    .meta = _ast_get_metadata(),
    .left = ids,
    .right = expr,
    .type = type,
  };

  return decl;
}

ConstantStmt *ast_constant(Identifier *id, Type *type, Expr *expr) {
  ConstantStmt *constant = new(ConstantStmt);
  *constant = (ConstantStmt){
    .meta = _ast_get_metadata(),
    .left = id,
    .right = expr,
    .type = type,
  };

  return constant;
}

TypeAliasStmt *ast_type_alias(Identifier *id, Type *type) {
  TypeAliasStmt *alias = new(TypeAliasStmt);
  *alias = (TypeAliasStmt){
    .meta = _ast_get_metadata(),
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
    .meta = _ast_get_metadata(),                  \
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
AST_EXPR_FUNC(ast_expr_import, ImportExpr, EXPR_IMPORT, import)
AST_EXPR_FUNC(ast_expr_env, EnvExpr, EXPR_ENV, env)

Expr *ast_expr_compose() {
  Expr *expr = new(Expr);
  *expr = (Expr){ .meta = _ast_get_metadata(), .type = EXPR_COMPOSE };
  return expr;
}

// -----------------------------------------------------------------------------

#define AST_LITERAL_FUNC(f_name, T, literal_t, p_name)  \
LiteralExpr *f_name(T *p_name) {                        \
  LiteralExpr *literal = new(LiteralExpr);              \
  *literal = (LiteralExpr){                             \
    .meta = _ast_get_metadata(),                        \
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
  literal->meta = _ast_get_metadata();
  literal->type = L_NIL;
  return literal;
}

LiteralExpr *ast_list_literal(ExprList *exprs) {
  ListLiteral *list = new(ListLiteral);
  list->meta = _ast_get_metadata();
  if (exprs) ast_consume_expr_list(exprs, &list->exprs, &list->len);

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .meta = _ast_get_metadata(),
    .type = L_LIST,
    .list = list
  };
  return literal;
}

LiteralExpr *ast_vector_literal(Expr *value, u32 count) {
  VectorLiteral *list = new(VectorLiteral);
  *list = (VectorLiteral){
    .meta = _ast_get_metadata(),
    .value = value,
    .len = count
  };

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .meta = _ast_get_metadata(),
    .type = L_VECTOR,
    .vec = list
  };
  return literal;
}

LiteralExpr *ast_map_literal(MapEntryList *entries) {
  MapLiteral *map = new(MapLiteral);
  map->meta = _ast_get_metadata();
  ast_consume_map_entry_list(entries, &map->entries, &map->len);

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .meta = _ast_get_metadata(),
    .type = L_MAP,
    .map = map
  };
  return literal;
}

LiteralExpr *ast_struct_literal(StructLiteralFieldList *fields) {
  StructLiteral *struct_literal = new(StructLiteral);
  struct_literal->meta = _ast_get_metadata();
  ast_consume_struct_literal_field_list(fields, &struct_literal->fields, &struct_literal->len);

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .meta = _ast_get_metadata(),
    .type = L_STRUCT,
    .struct_literal = struct_literal
  };
  return literal;
}

LiteralExpr *ast_range_literal(Expr *start, Expr *end, bool inclusive) {
  RangeLiteral *range = new(RangeLiteral);
  *range = (RangeLiteral){
    .meta = _ast_get_metadata(),
    .start = start,
    .end = end,
    .inclusive = inclusive
  };

  LiteralExpr *literal = new(LiteralExpr);
  *literal = (LiteralExpr){
    .meta = _ast_get_metadata(),
    .type = L_RANGE,
    .range = range
  };
  return literal;
}

// -----------------------------------------------------------------------------

VariableExpr *ast_variable_named(Identifier *id) {
  NamedVariable *named_var = new(NamedVariable);
  *named_var = (NamedVariable){
    .meta = _ast_get_metadata(),
    .name = id
  };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .meta = _ast_get_metadata(),
    .type = V_NAMED,
    .named = named_var,
  };

  return var;
}

VariableExpr *ast_variable_struct_member(VariableExpr *struct_expr, Identifier *id) {
  StructMemberVariable *struct_var = new(StructMemberVariable);
  *struct_var = (StructMemberVariable){
    .meta = _ast_get_metadata(),
    .struct_expr = struct_expr,
    .name = id,
  };

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .meta = _ast_get_metadata(),
    .type = V_STRUCT_MEMBER,
    .struct_member = struct_var,
  };

  return var;
}

VariableExpr *ast_variable_indexed(VariableExpr *container, ExprList *indices) {
  IndexedVariable *idx_var = new(IndexedVariable);
  *idx_var = (IndexedVariable){
    .meta = _ast_get_metadata(),
    .container = container
  };
  ast_consume_expr_list(indices, &idx_var->indices, &idx_var->index_count);

  VariableExpr *var = new(VariableExpr);
  *var = (VariableExpr){
    .meta = _ast_get_metadata(),
    .type = V_INDEX,
    .indexed = idx_var,
  };

  return var;
}

// -----------------------------------------------------------------------------

UnaryExpr *ast_unary(TokenLabel op, Expr *expr) {
  UnaryExpr *unary = new(UnaryExpr);
  *unary = (UnaryExpr){
    .meta = _ast_get_metadata(),
    .op = op,
    .expr = expr,
  };

  return unary;
}

BinaryExpr *ast_binary(Expr *left, TokenLabel op, Expr *right) {
  BinaryExpr *binary = new(BinaryExpr);
  *binary = (BinaryExpr){
    .meta = _ast_get_metadata(),
    .left = left,
    .op = op,
    .right = right,
  };

  return binary;
}

GroupExpr *ast_group(Expr *expr) {
  GroupExpr *group = new(GroupExpr);
  *group = (GroupExpr){
    .meta = _ast_get_metadata(),
    .inner_expr = expr
  };

  return group;
}

AssignmentExpr *ast_assignment(VariableExpr *left, Expr *right) {
  AssignmentExpr *assign = new(AssignmentExpr);
  *assign = (AssignmentExpr){
    .meta = _ast_get_metadata(),
    .left = left,
    .right = right,
  };

  return assign;
}

IfExpr *ast_if(Expr *condition, Expr *true_branch, Expr *false_branch) {
  IfExpr *if_expr = new(IfExpr);
  *if_expr = (IfExpr){
    .meta = _ast_get_metadata(),
    .condition = condition,
    .true_branch = true_branch,
    .false_branch = false_branch,
  };

  return if_expr;
}

ForExpr *ast_for(Identifier *var, Identifier *idx, Expr *iterable, Expr *body) {
  ForExpr *for_expr = new(ForExpr);
  *for_expr = (ForExpr){
    .meta = _ast_get_metadata(),
    .var_id = var,
    .idx_id = idx,
    .iterable = iterable,
    .body = body,
  };

  return for_expr;
}

BlockExpr *ast_block(StmtList *statements) {
  BlockExpr* block = new(BlockExpr);
  block->meta = _ast_get_metadata();
  ast_consume_stmt_list(statements, &block->statements, &block->len);
  return block;
}

FunctionCallExpr *ast_function_call(Identifier *id, ArgumentList *args, Expr *composable) {
  FunctionCallExpr *function_call = new(FunctionCallExpr);
  *function_call = (FunctionCallExpr){
    .meta = _ast_get_metadata(),
    .name = id,
    .composable = composable,
  };
  ast_consume_argument_list(args, &function_call->args, &function_call->args_len);

  return function_call;
}

ImportExpr *ast_import(ImportType type, Type *data_type, char *path) {
  ImportExpr *import = new(ImportExpr);
  *import = (ImportExpr){
    .meta = _ast_get_metadata(),
    .type = type,
    .data_type = data_type,
    .path = path,
  };

  return import;
}

EnvExpr *ast_env(StringLiteral *env_name, Expr *default_value) {
  EnvExpr *env = new(EnvExpr);
  *env = (EnvExpr){
    .meta = _ast_get_metadata(),
    .env_name = env_name,
    .default_value = default_value,
  };

  return env;
}

// -----------------------------------------------------------------------------

Type *ast_named_type(Identifier *id) {
  NamedType *named_type = new(NamedType);
  *named_type = (NamedType){
    .meta = _ast_get_metadata(),
    .name = id
  };

  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_NAMED,
    .named = named_type
  };

  return type;
}

Type *ast_list_type(Type *item_type) {
  ListType *list = new(ListType);
  *list = (ListType){ .item_type = item_type };

  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_LIST,
    .list = list
  };

  return type;
}

Type *ast_map_type(Type *key_type, Type *value_type) {
  MapType *map = new(MapType);
  *map = (MapType){
    .meta = _ast_get_metadata(),
    .key_type = key_type,
    .value_type = value_type,
  };

  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_MAP,
    .map = map
  };

  return type;
}

Type *ast_struct_type(StructFieldList *field_list) {
  StructType *struct_type = new(StructType);
  *struct_type = (StructType){
    .meta = _ast_get_metadata(),
    .fields = new(StructFieldMap),
  };
  map_init(struct_type->fields, hash_cstring, compare_cstring);
  map_manage_values(struct_type->fields, ast_free_struct_field);

  StructField **fields;
  u32 field_count;
  ast_consume_struct_field_list(field_list, &fields, &field_count);
  for (u32 i = 0; i < field_count; i++) {
    char *name = fields[i]->name->lexeme;
    if (!map_insert(struct_type->fields, name, fields[i])) {
      const StructField *prev = map_get(struct_type->fields, name);
      err_report_struct_field(
        strdup(name),
        &prev->name->loc,
        &fields[i]->name->loc,
        ERR_ERROR
      );
      ast_free_struct_field(fields[i]);
    }
  }
  free(fields);

  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_STRUCT,
    .struct_type = struct_type
  };

  return type;
}

Type *ast_union_type(TypeList *types) {
  UnionType *union_type = new(UnionType);
  union_type->meta = _ast_get_metadata();
  ast_consume_type_list(types, &union_type->types, &union_type->len);

  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_UNION,
    .union_type = union_type
  };

  return type;
}

Type *ast_tuple_type(TypeList *types) {
  TupleType *tuple = new(TupleType);
  tuple->meta = _ast_get_metadata();
  ast_consume_type_list(types, &tuple->types, &tuple->len);

  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_TUPLE,
    .tuple = tuple
  };

  return type;
}

Type *ast_vector_type(Type *inner_type, u32 count) {
  VectorType *tuple = new(VectorType);
  *tuple = (VectorType){
    .meta = _ast_get_metadata(),
    .item_type = inner_type,
    .len = count
  };

  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_VECTOR,
    .vec = tuple
  };

  return type;
}

Type *ast_builtin_type(BuiltinType builtin_type) {
  Type *type = new(Type);
  *type = (Type){
    .meta = _ast_get_metadata(),
    .type = T_BUILTIN,
    .builtin = builtin_type,
  };

  return type;
}

// -----------------------------------------------------------------------------

FunctionDef *ast_function_def(
  Identifier *id,
  ParameterList *params,
  Type *compose_type,
  Type *return_type,
  BlockExpr *body
) {
  FunctionDef *function = new(FunctionDef);
  *function = (FunctionDef){
    .meta = _ast_get_metadata(),
    .name = id,
    .compose_type = compose_type,
    .return_type = return_type,
    .body = body,
  };
  if (params) {
    ast_consume_parameter_list(params, &function->params, &function->params_len);
  }

  return function;
}

// -----------------------------------------------------------------------------

StructField *ast_struct_field(Identifier *id, Type *type) {
  StructField *field = new(StructField);
  *field = (StructField){
    .meta = _ast_get_metadata(),
    .name = id,
    .type = type,
  };

  return field;
}

StructLiteralField *ast_struct_literal_field(Identifier *id, Expr *value) {
  StructLiteralField *field = new(StructLiteralField);
  *field = (StructLiteralField){
    .meta = _ast_get_metadata(),
    .name = id,
    .value = value,
  };

  return field;
}

MapEntry *ast_map_entry(Expr *key, Expr *value) {
  MapEntry *entry = new(MapEntry);
  *entry = (MapEntry){
    .meta = _ast_get_metadata(),
    .key = key,
    .value = value,
  };

  return entry;
}

Parameter *ast_parameter(Identifier *id, Type *type, Expr *default_value) {
  Parameter *param = new(Parameter);
  *param = (Parameter){
    .meta = _ast_get_metadata(),
    .name = id,
    .type = type,
    .default_value = default_value,
  };

  return param;
}

Argument *ast_argument(Identifier *id, Expr *value) {
  Argument *arg = new(Argument);
  *arg = (Argument){
    .meta = _ast_get_metadata(),
    .name = id,
    .value = value,
  };

  return arg;
}

// -----------------------------------------------------------------------------

Program *ast_program(StmtList *statements) {
  Program *prog = new(Program);
  prog->meta = _ast_get_metadata();
  ast_consume_stmt_list(statements, &prog->statements, &prog->len);
  return prog;
}

// -----------------------------------------------------------------------------

CSVString *ast_csv_string(str value) {
  CSVString *string = new(CSVString);
  *string = (CSVString){
    .meta = _ast_get_metadata(),
    .value = value,
  };
  return string;
}

CSVRecord *ast_csv_record(CSVStringList *fields) {
  CSVRecord *record = new(CSVRecord);
  record->meta = _ast_get_metadata();
  ast_consume_csv_string_list(fields, &record->fields, &record->len);
  return record;
}

CSVFile *ast_csv_file(CSVRecordList *record_list) {
  u32 record_count;
  CSVRecord **records;
  ast_consume_csv_record_list(record_list, &records, &record_count);
  if (record_count == 0) {
    char *msg = strdup("file is empty");
    err_report_import_csv(msg, &record_list->meta.loc, ERR_ERROR);
    return NULL;
  }

  u32 columns = records[0]->len;
  u32 rows = record_count - 1;
  CSVString **headers = new_array(CSVString *, columns);
  CSVString **data = new_array(CSVString *, columns * rows);

  for (u32 j = 0; j < columns; j++) {
    headers[j] = records[0]->fields[j];
  }
  free(records[0]->fields);
  free(records[0]);
  for (u32 i = 0; i < rows; i++) {
    CSVRecord *row = records[i + 1];
    if (row->len != columns) {
      char *msg = NULL;
      asprintf(&msg,
        "column count mismatch at row %u: expected %u, got %u",
        i + 1, columns, row->len
      );
      err_report_import_csv(msg, &row->meta.loc, ERR_ERROR);
      ast_free_csv_record(row);
    } else {
      for (u32 j = 0; j < columns; j++) {
        data[i * columns + j] = row->fields[j];
      }
      free(row->fields);
      free(row);
    }
  }

  free(records);
  CSVFile *file = new(CSVFile);
  *file = (CSVFile){
    .meta = _ast_get_metadata(),
    .columns = columns,
    .rows = rows,
    .headers = headers,
    .data = data,
  };
  return file;
}

// -----------------------------------------------------------------------------

bool ast_compare_identifiers(const Identifier *a, const Identifier *b) {
  return a->len == b->len && strncmp(a->lexeme, b->lexeme, a->len) == 0;
}

Identifier *ast_clone_identifier(const Identifier *id) {
  if (!id) return NULL;

  Identifier *clone = new(Identifier);
  *clone = (Identifier){
    .label = id->label,
    .len = id->len,
    .lexeme = strdup(id->lexeme),
    .loc = id->loc,
  };

  return clone;
}

// -----------------------------------------------------------------------------

void ast_free_stmt(Stmt *stmt) {
  if (!stmt) return;

  switch (stmt->type) {
    case STMT_EXPR: ast_free_expr(stmt->expr); break;
    case STMT_DECLARATION: ast_free_decl(stmt->decl); break;
    case STMT_CONSTANT: ast_free_constant(stmt->constant); break;
    case STMT_TYPE_ALIAS: ast_free_alias(stmt->type_alias); break;
    case STMT_FUNCTION_DEF: ast_free_function_def(stmt->function_def); break;
    case STMT_EXPORT: ast_free_export_directive(stmt->export); break;
    case STMT_IMPORT: ast_free_import_directive(stmt->import); break;
    case STMT_PARSE_ERROR: break;
  }

  free(stmt);
}

void ast_free_decl(DeclarationStmt *decl) {
  if (!decl) return;

  for (u32 i = 0; i < decl->left_count; i++) {
    ast_free_identifier(decl->left[i]);
  }
  free(decl->left);
  ast_free_expr(decl->right);
  ast_free_type(decl->type);
  free(decl);
}

void ast_free_constant(ConstantStmt *constant) {
  if (!constant) return;

  ast_free_identifier(constant->left);
  ast_free_expr(constant->right);
  ast_free_type(constant->type);
  free(constant);
}


void ast_free_partial_decl(PartialDeclarationStmt *decl) {
  if (!decl) return;

  ast_free_identifier_list(decl->left, true);
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

void ast_free_export_directive(ExportDirective *e) {
  if (!e) return;

  for (u32 i = 0; i < e->len; i++) {
    ast_free_identifier(e->ids[i]);
  }
  free(e->ids);
  free(e);
}

void ast_free_import_directive(ImportDirective *im) {
  if (!im) return;

  for (u32 i = 0; i < im->len; i++) {
    ast_free_identifier(im->ids[i]);
  }
  free(im->ids);
  free(im);
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
    case EXPR_IMPORT: ast_free_import(expr->import); break;
    case EXPR_ENV: ast_free_env(expr->env); break;
    case EXPR_COMPOSE: break;
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
    case L_VECTOR: ast_free_vector_literal(literal->vec); break;
    case L_MAP: ast_free_map_literal(literal->map); break;
    case L_STRUCT: ast_free_struct_literal(literal->struct_literal); break;
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

void ast_free_import(ImportExpr *import) {
  if (!import) return;

  ast_free_type(import->data_type);
  if (import->path) free(import->path);
  switch (import->type) {
    case IMPORT_CSV: ast_free_csv_file(import->csv); break;
    default: break;
  }

  free(import);
}

void ast_free_env(EnvExpr *env) {
  if (!env) return;

  ast_free_string_literal(env->env_name);
  ast_free_expr(env->default_value);
  free(env);
}

void ast_free_int_literal(IntegerLiteral *l) {
  if (!l) return;

  ast_free_token(l->token);
  free(l);
}

void ast_free_float_literal(FloatLiteral *l) {
  if (!l) return;

  ast_free_token(l->token);
  free(l);
}

void ast_free_string_literal(StringLiteral *l) {
  if (!l) return;

  str_free(l->value);
  ast_free_token(l->token);
  free(l);
}

void ast_free_bool_literal(BooleanLiteral *l) {
  if (!l) return;

  ast_free_token(l->token);
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

void ast_free_vector_literal(VectorLiteral *l) {
  if (!l) return;

  ast_free_expr(l->value);
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
  for (u32 i = 0; i < v->index_count; i++) {
    ast_free_expr(v->indices[i]);
  }
  free(v->indices);
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
    case T_VECTOR: ast_free_vector_type(t->vec); break;
    case T_BUILTIN: break;
    case T_RANGE: break;
    case T_ERROR: break;
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

  map_free(t->fields);
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

void ast_free_vector_type(VectorType *t) {
  if (!t) return;

  ast_free_type(t->item_type);
  free(t);
}

void ast_free_function_def(FunctionDef *f) {
  if (!f) return;

  ast_free_identifier(f->name);
  for (u32 i = 0; i < f->params_len; i++) {
    ast_free_parameter(f->params[i]);
  }
  ast_free_type(f->compose_type);
  ast_free_type(f->return_type);
  ast_free_block(f->body);
  free(f->params);
  free(f);
}

void ast_free_struct_field(StructField *f) {
  if (!f) return;

  ast_free_identifier(f->name);
  ast_free_type(f->type);
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

void ast_free_csv_string(CSVString *string) {
  if (!string) return;

  str_free(string->value);
  free(string);
}

void ast_free_csv_record(CSVRecord *record) {
  if (!record) return;

  for (u32 i = 0; i < record->len; i++) {
    ast_free_csv_string(record->fields[i]);
  }
  free(record->fields);
  free(record);
}

void ast_free_csv_file(CSVFile *file) {
  if (!file) return;

  for (u32 j = 0; j < file->columns; j++) {
    ast_free_csv_string(file->headers[j]);
    for (u32 i = 0; i < file->rows; i++) {
      ast_free_csv_string(file->data[i * file->columns + j]);
    }
  }
  if (file->coercion_hints) free(file->coercion_hints);
  free(file->headers);
  free(file->data);
  free(file);
}
