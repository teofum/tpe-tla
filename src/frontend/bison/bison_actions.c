#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YY_HEADER_EXPORT_START_CONDITIONS
#include <ast/ast.h>
#include <error/error.h>
#include <frontend/flex/flex_scanner.h>
#include <frontend/frontend.h>
#include <support/logger.h>
#include <support/str.h>
#include <support/types.h>
#include <support/util.h>

#include "bison_actions.h"
#include "bison_parser.h"

#define PATH_MAX 4096

Stmt *parse_expr_stmt(Expr *expr) {
  fe_parser_log(LOG_DEBUG, "Expression Stmt");
  return ast_stmt_expr(expr);
}

Stmt *parse_declaration_stmt(PartialDeclarationStmt *decl) {
  fe_parser_log(LOG_DEBUG, "Declaration Stmt");
  return ast_stmt_decl(decl);
}

Stmt *parse_constant_stmt(ConstantStmt *constant) {
  fe_parser_log(LOG_DEBUG, "Constant Stmt");
  return ast_stmt_constant(constant);
}

Stmt *parse_type_alias_stmt(TypeAliasStmt *alias) {
  fe_parser_log(LOG_DEBUG, "Type Alias Stmt");
  return ast_stmt_alias(alias);
}

Stmt *parse_function_def_stmt(FunctionDef *function) {
  fe_parser_log(LOG_DEBUG, "Function Definition Stmt");
  return ast_stmt_function_def(function);
}

static char *_resolve_import_path(str rel_path) {
  bool from_root = rel_path.len > 0 && rel_path.ptr[0] == '/';
  char * base_cstr = from_root
    ? cs()->options.base_path
    : dirname(strdup(cs_current_filepath()));

  str base_path = str_from_cstring(base_cstr);
  fe_parser_log(LOG_DEBUG, "rel '%.*s'", (i32)rel_path.len, rel_path.ptr);
  fe_parser_log(LOG_DEBUG, "base '%.*s'", (i32)base_path.len, base_path.ptr);
  if (!from_root) free(base_cstr);

  str sep = str_from_cstring("/");
  str full_path = str_concat(2, sep, base_path, rel_path);
  fe_parser_log(LOG_DEBUG, "full '%.*s'", (i32)full_path.len, full_path.ptr);
  str_free(base_path);
  str_free(sep);

  char *full_path_cstr = str_to_cstring(full_path);
  str_free(full_path);

  char *real_path = realpath(full_path_cstr, NULL);
  free(full_path_cstr);
  return real_path;
}

Stmt *parse_import_directive(IdentifierList *ids, StringLiteral *from) {
  fe_parser_log(LOG_DEBUG, "Import Directive");

  CompilerOptions *o = &cs()->options;
  i32 file_idx = -1;
  char *real_from_path = _resolve_import_path(from->value);
  if (real_from_path) {
    for (u32 i = 0; i < o->input_file_count && file_idx == -1; i++) {
      char *real_file_path = realpath(o->input_filenames[i], NULL);
      if (real_file_path && strcmp(real_file_path, real_from_path) == 0) {
        cs()->file_dependencies[cs()->current_file][i] = true;
        file_idx = i;
      }
      if (real_file_path) free(real_file_path);
    }
    free(real_from_path);
  }
  if (file_idx == -1) {
    err_report_import_file(
      str_to_cstring(from->value),
      &from->token->loc,
      ERR_ERROR
    );
  }

  ast_free_string_literal(from);
  return ast_stmt_import(ids, file_idx);
}

Stmt *parse_export_directive(IdentifierList *ids) {
  fe_parser_log(LOG_DEBUG, "Export Directive");
  return ast_stmt_export(ids);
}

Stmt *parse_error_stmt() {
  fe_parser_log(LOG_DEBUG, "ERROR Stmt");
  return ast_stmt_error();
}

PartialDeclarationStmt *parse_declaration(Identifier *left, Type *type, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Declaration for %s", left->lexeme);

  IdentifierList *ids = new(IdentifierList);
  *ids = (IdentifierList){ .len = 1, .head = left, .tail = NULL };
  return ast_declaration(ids, type, right);
}

PartialDeclarationStmt *parse_declaration_decomp(Identifier *left, PartialDeclarationStmt *base) {
  fe_parser_log(LOG_DEBUG, "Declaration for %s", left->lexeme);

  Type *type = base->type;
  Expr *right = base->right;
  IdentifierList *ids = base->left;
  free(base);

  IdentifierList *last = ids;
  while (last->tail) {
    last->len += 1;
    last = last->tail;
  }
  last->len = 2;
  last->tail = new(IdentifierList);
  *last->tail = (IdentifierList){ .len = 1, .head = left, .tail = NULL };

  return ast_declaration(ids, type, right);
}

ConstantStmt *parse_constant(Identifier *left, Type *type, Expr *right) {
  fe_parser_log(LOG_DEBUG, "Constant declaration for %s", left->lexeme);
  return ast_constant(left, type, right);
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
PARSE_EXPR_FUNC(function_call, FunctionCallExpr, "Function Call Expr %s", expr->name->lexeme)
PARSE_EXPR_FUNC(env, EnvExpr, "Env Expr %s", expr->env_name->token->lexeme)

Expr *parse_import_expr(ImportExpr *expr, CSVFile *csv) {
  fe_parser_log(LOG_DEBUG, "Import Expr %s", expr->path);
  expr->csv = csv;
  return ast_expr_import(expr);
}

Expr *parse_compose_expr() {
  fe_parser_log(LOG_DEBUG, "Compose Expr");
  return ast_expr_compose();
}

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
  fe_parser_log(LOG_DEBUG, "String Literal %s", s->token->lexeme);
  return ast_string_literal(s);
}

LiteralExpr *parse_boolean_literal(BooleanLiteral *b) {
  fe_parser_log(LOG_DEBUG, "Boolean Literal %s", b->token->lexeme);
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

LiteralExpr *parse_vector_literal(Expr *value, IntegerLiteral *count) {
  LiteralExpr *literal = ast_vector_literal(value, (u32)count->value);
  fe_parser_log(LOG_DEBUG, "Shorthand List Literal", literal->list->len);
  ast_free_int_literal(count);
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

LiteralExpr *parse_range_literal(Expr *start, Expr *end, bool inclusive) {
  fe_parser_log(LOG_DEBUG, "Range Literal");
  return ast_range_literal(start, end, inclusive);
}

Expr *parse_range_value_int(IntegerLiteral *i) {
  return parse_literal_expr(parse_integer_literal(i));
}

Expr *parse_range_value_var(VariableExpr *var) {
  return parse_variable_expr(var);
}

Expr *parse_range_value_group(GroupExpr *group) {
  return parse_group_expr(group);
}


// -----------------------------------------------------------------------------

VariableExpr *parse_named_variable(Identifier *id) {
  fe_parser_log(LOG_DEBUG, "Named Variable %s", id->lexeme);
  return ast_variable_named(id);
}

VariableExpr *parse_struct_member_variable(VariableExpr *struct_expr, IdentifierList *ids) {
  Identifier *id = ids->head;
  fe_parser_log(LOG_DEBUG, "Struct Member %s", id->lexeme);

  if (ids->tail) {
    struct_expr = parse_struct_member_variable(struct_expr, ids->tail);
    ids->tail = NULL; // Recursive call freed the tail, set it to NULL to avoid double free
  }
  ast_free_identifier_list(ids, false);

  return ast_variable_struct_member(struct_expr, id);
}

VariableExpr *parse_indexed_variable(VariableExpr *container, ExprList *indices) {
  fe_parser_log(LOG_DEBUG, "Indexed variable");
  return ast_variable_indexed(container, indices);
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

ImportExpr *parse_import(Identifier *import_type, Type *data_type, StringLiteral *path) {
  fe_parser_log(LOG_DEBUG, "Import %s '%s'", import_type->lexeme, path->token->lexeme);

  ImportType type = IMPORT_UNKNOWN;
  for (u32 i = 0; i < IMPORT_TYPE_COUNT; i++) {
    if (strcmp(import_type->lexeme, import_type_str[i]) == 0) type = (ImportType)i;
  }

  char *resolved_path = NULL;
  if (type == IMPORT_UNKNOWN) {
    ImportError *error = new(ImportError);
    error->type = strdup(import_type->lexeme);
    err_report_import(error, &import_type->loc, ERR_ERROR);
    fe_parser_log(LOG_ERROR, "Unknown import type '%s'", import_type->lexeme);
  } else if (type != IMPORT_CSV && data_type != NULL) {
    ImportError *error = new(ImportError);
    error->type = strdup(import_type->lexeme);
    err_report_typed_import(error, &data_type->meta.loc, ERR_ERROR);
    fe_parser_log(LOG_ERROR, "Import type '%s' does not support type annotations", import_type->lexeme);
  } else {
    resolved_path = _resolve_import_path(path->value);
    if (!resolved_path) {
      fe_parser_log(LOG_ERROR, "File '%.*s' does not exist", path->value.len, path->value.ptr);
      err_report_import_file(str_to_cstring(path->value), &path->token->loc, ERR_ERROR);
    } else {
      fe_parser_log(LOG_INFO, "Resolved import %s '%s'", import_type_str[type], resolved_path);
    }
  }

  ast_free_identifier(import_type);
  ast_free_string_literal(path);

  if (type == IMPORT_CSV) {
    fe_push_buffer(resolved_path);
    fe_enter_context(LEX_CSV);
  }

  return ast_import(type, data_type, resolved_path);
}

EnvExpr *parse_env(StringLiteral *env_name, Expr *default_value) {
  fe_parser_log(LOG_DEBUG, "Env %s", env_name->token->lexeme);
  return ast_env(env_name, default_value);
}

// -----------------------------------------------------------------------------

Type *parse_named_type(Identifier *id) {
  BuiltinType builtin = fe_get_builtin_type(id);
  if (builtin == -1) {
    fe_parser_log(LOG_DEBUG, "Named Type %s", id->lexeme);
    return ast_named_type(id);
  } else {
    fe_parser_log(LOG_DEBUG, "Builtin Type %s", id->lexeme);
    ast_free_identifier(id);
    return ast_builtin_type(builtin);
  }
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
  TypeList *types = ast_type_list(type, ast_type_list(ast_builtin_type(T_NIL), NULL));
  return ast_union_type(types);
}

Type *parse_tuple_type(TypeList *types) {
  fe_parser_log(LOG_DEBUG, "Tuple");
  return ast_tuple_type(types);
}

Type *parse_vector_type(Type *type, IntegerLiteral *count) {
  fe_parser_log(LOG_DEBUG, "Tuple Shorthand");
  Type *tuple = ast_vector_type(type, (u32)count->value);
  ast_free_int_literal(count);
  return tuple;
}

Type *parse_nil_type() {
  fe_parser_log(LOG_DEBUG, "Nil Type");
  return ast_builtin_type(T_NIL);
}

// -----------------------------------------------------------------------------

FunctionDef *parse_function_def(
  Identifier *id,
  ParameterList *params,
  Type *compose_type,
  Type *return_type,
  BlockExpr *body
) {
  fe_parser_log(LOG_DEBUG, "Function Definition %s", id->lexeme);
  return ast_function_def(id, params, compose_type, return_type, body);
}

static Argument *_parse_argument(Expr *expr) {
  Argument *arg = NULL;

  if (expr->type == EXPR_ASSIGNMENT) {
    VariableExpr *var = expr->assignment->left;
    if (var->type == V_NAMED) {
      arg = ast_argument(var->named->name, expr->assignment->right);
      free(var->named);
      free(var);
      free(expr->assignment);
      free(expr);
    } else {
      const char *expected = "identifier";

      SyntaxErrorContext *ctx = new(SyntaxErrorContext);
      *ctx = (SyntaxErrorContext) {
        .expected_token_count = 1,
        .expected_tokens = &expected,
        .found_token = "variable path",
        .location = &expr->meta.loc,
      };

      fe_report_syntax_error(ctx);
      ast_free_expr(expr);
      arg = ast_argument(NULL, NULL);
    }
  } else {
    arg = ast_argument(NULL, expr);
  }

  return arg;
}

static Expr *_id_to_string(Identifier *id) {
  StringLiteral *str_l = new(StringLiteral);
  *str_l = (StringLiteral){ .token = id, .value = str_from_cstring(id->lexeme) };

  return ast_expr_literal(ast_string_literal(str_l));
}

static ExprList *_id_to_string_list(IdentifierList *id_list) {
  if (!id_list) return NULL;

  ExprList *exprs = ast_expr_list(_id_to_string(id_list->head), _id_to_string_list(id_list->tail));
  id_list->tail = NULL;
  ast_free_identifier_list(id_list, false);

  return exprs;
}

static ArgumentList *_append_arg(ArgumentList *args, const char *name, Expr *value) {
  usize name_len = strlen(name);
  Identifier *name_id = new(Identifier);
  *name_id = (Identifier){ .label = IDENTIFIER, .len = name_len, .lexeme = strndup(name, name_len) };

  Argument *class_arg = new(Argument);
  *class_arg = (Argument) { .name = name_id, .value = value };

  ArgumentList *args_with_class = new(ArgumentList);
  *args_with_class = (ArgumentList){
    .len = args->len + 1,
    .tail = args,
    .head = class_arg,
  };
  return args_with_class;
}

FunctionCallExpr *parse_function_call(Identifier *id, ExprList *arg_exprs, Expr *composable, IdentifierList *classlist, Identifier *html_id) {
  fe_parser_log(LOG_DEBUG, "Function Call %s", id->lexeme);

  ArgumentList *args = new(ArgumentList);
  ArgumentList *args_tail = args;
  ExprList *expr_list = arg_exprs;
  while (expr_list) {
    Argument *arg = _parse_argument(expr_list->head);
    args_tail->head = arg;

    args_tail->len = expr_list->len;
    if (expr_list->tail) {
      args_tail->tail = new(ArgumentList);
      args_tail = args_tail->tail;
    }
    expr_list = expr_list->tail;
  }

  if (classlist) {
    // Convert classlist to string literals
    ExprList *class_string_list = _id_to_string_list(classlist);
    Expr *class_value = ast_expr_literal(ast_list_literal(class_string_list));
    args = _append_arg(args, "class", class_value);
  }
  if (html_id) {
    args = _append_arg(args, "id", _id_to_string(html_id));
  }

  ast_free_expr_list(arg_exprs, false);
  return ast_function_call(id, args, composable);
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
  if (!head) return tail;
  StmtList* list = ast_stmt_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Stmt List (len=%u)", list->len);
  return list;
}

ExprList *parse_expr_list(Expr *head, ExprList *tail) {
  ExprList* list = ast_expr_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Expr List (len=%u)", list->len);
  return list;
}

ParameterList *parse_parameter_list(Parameter *head, ParameterList *tail) {
  ParameterList* list = ast_parameter_list(head, tail);
  fe_parser_log(LOG_DEBUG, "Parameter List (len=%u)", list->len);
  return list;
}

// -----------------------------------------------------------------------------

StructField *parse_struct_field(Identifier *id, Type *type) {
  fe_parser_log(LOG_DEBUG, "Struct Field %s", id->lexeme);
  return ast_struct_field(id, type);
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

Parameter *parse_parameter(Identifier *id, Type *type, Expr *default_value) {
  fe_parser_log(LOG_DEBUG, "Parameter %s", id->lexeme);
  return ast_parameter(id, type, default_value);
}

// -----------------------------------------------------------------------------

Program *parse_program(StmtList *statements) {
  Program *prog = ast_program(statements);
  fe_parser_log(LOG_DEBUG, "Program (len=%u)", prog->len);
  fe_set_ast(prog);
  return prog;
}

// -----------------------------------------------------------------------------

CSVString *parse_csv_str_literal(const char *value) {
  CSVString *l = new(CSVString);
  *l = (CSVString){
    .meta = {0},
    .value = str_from_cstring(value),
  };
  return l;
}

CSVString *parse_csv_str_concat(CSVString *l, CSVString *r) {
  str_append(&l->value, r->value);
  ast_free_csv_string(r);
  return l;
}

CSVStringList *parse_csv_field_list(CSVString *head, CSVStringList *tail) {
  CSVStringList *list = ast_csv_string_list(head, tail);
  fe_parser_log(LOG_DEBUG, "CSV Field List (len=%u)", list->len);
  return list;
}

CSVRecord *parse_csv_record(CSVStringList *fields) {
  fe_parser_log(LOG_DEBUG, "CSV Record");
  return ast_csv_record(fields);
}

CSVRecordList *parse_csv_record_list(CSVRecord *head, CSVRecordList *tail) {
  CSVRecordList *list = ast_csv_record_list(head, tail);
  fe_parser_log(LOG_DEBUG, "CSV Record List (len=%u)", list->len);
  return list;
}

CSVFile *parse_csv_file(CSVRecordList *records) {
  fe_parser_log(LOG_DEBUG, "CSV File");
  return ast_csv_file(records);
}

// -----------------------------------------------------------------------------

void parse_error(Location *loc, const char *message) {
  fe_parser_log(LOG_ERROR, "%s\n\tAt line %u, col %u", message, loc->first_line, loc->first_column);
}

void parse_set_location(Location *loc) {
  ast_set_next_location(loc);
}
