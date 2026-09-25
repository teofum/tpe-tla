#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ast/ast.h>
#include <frontend/frontend.h>
#include <support/str.h>
#include <support/types.h>
#include <support/util.h>

#include "dot.h"

typedef enum {
  NODE_ROOT,
  NODE_STMT,
  NODE_BASE,
  NODE_TYPE,
  NODE_ERROR,
} NodeType;

static const char *attrs_for_type[] = {
  [NODE_ROOT] = ",style=filled,fillcolor=\"#a0c0ff\"",
  [NODE_BASE] = ",style=filled",
  [NODE_STMT] = ",style=filled,fillcolor=\"#a0ffc0\"",
  [NODE_TYPE] = ",style=filled,fillcolor=\"#ffc0ff\"",
  [NODE_ERROR] = ",style=filled,fillcolor=\"#ffc0c0\"",
};

// Internals
static FILE *_of = NULL;
static u64 _id = 0;

static u64 next_id() {
  return _id++;
}

static void _printfln(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  u64 len = strlen(fmt);
  char *fmt_ln = new_array(char, len + 2);
  strncpy(fmt_ln, fmt, len);
  fmt_ln[len] = '\n';

  vfprintf(_of, fmt_ln, args);
  free(fmt_ln);
  va_end(args);
}

static void _node(u64 id, const char *label, NodeType type) {
  _printfln("%llu [shape=box,label=\"%s\"%s];", id, label, attrs_for_type[type]);
}

static void _edge(u64 id1, u64 id2) {
  _printfln("%llu -> %llu;", id1, id2);
}

static void dot_stmt(Stmt *stmt, u64 pid);
static void dot_expr(Expr *expr, u64 pid);
static void dot_type(Type *type, u64 pid);

static void dot_struct_field(StructField *field, u64 pid) {
  u64 id = next_id();
  char label[256];
  snprintf(label, 256, "%s", field->name->lexeme);
  _node(id, label, NODE_BASE);
  _edge(pid, id);
  dot_type(field->type, id);
  if (field->default_value) dot_expr(field->default_value, id);
}

static void dot_struct_literal_field(StructLiteralField *field, u64 pid) {
  u64 id = next_id();
  char label[256];
  snprintf(label, 256, "%s", field->name->lexeme);
  _node(id, label, NODE_BASE);
  _edge(pid, id);
  dot_expr(field->value, id);
}

static void dot_map_entry(MapEntry *entry, u64 pid) {
  u64 id = next_id();
  _node(id, "Map Entry", NODE_BASE);
  _edge(pid, id);
  dot_expr(entry->key, id);
  dot_expr(entry->value, id);
}

static void dot_enum_value(Identifier *value, u64 pid) {
  u64 id = next_id();
  char label[256];
  snprintf(label, 256, "%s", value->lexeme);
  _node(id, label, NODE_BASE);
  _edge(pid, id);
}

static void dot_type(Type *type, u64 pid) {
  u64 id = next_id();
  char label[256];

  switch (type->type) {
    case T_NAMED:
      snprintf(label, 256, "%s", type->named->name->lexeme);
      break;
    case T_LIST:
      snprintf(label, 8, "List of");
      dot_type(type->list->item_type, id);
      break;
    case T_MAP:
      snprintf(label, 8, "Map of");
      dot_type(type->map->key_type, id);
      dot_type(type->map->value_type, id);
      break;
    case T_STRUCT:
      snprintf(label, 8, "Struct");
      for (u32 i = 0; i < type->struct_type->len; i++) {
        dot_struct_field(type->struct_type->fields[i], id);
      }
      break;
    case T_UNION:
      snprintf(label, 8, "Union");
      for (u32 i = 0; i < type->union_type->len; i++) {
        dot_type(type->union_type->types[i], id);
      }
      break;
    case T_TUPLE:
      snprintf(label, 8, "Tuple");
      for (u32 i = 0; i < type->tuple->len; i++) {
        dot_type(type->tuple->types[i], id);
      }
      break;
    case T_ENUM:
      snprintf(label, 8, "Enum");
      for (u32 i = 0; i < type->enum_type->len; i++) {
        dot_enum_value(type->enum_type->values[i], id);
      }
      break;
    case T_NIL:
      snprintf(label, 4, "Nil");
      break;
  }

  _node(id, label, NODE_TYPE);
  _edge(pid, id);
}

static void dot_literal(LiteralExpr *literal, u64 pid) {
  u64 id = next_id();
  char *label;

  switch (literal->type) {
    case L_INTEGER:
      label = new_array(char, 256);
      snprintf(label, 256, "Integer\\n%lld", (long long)literal->integer->value);
      break;
    case L_FLOAT:
      label = new_array(char, 256);
      snprintf(label, 256, "Float\\n%f", literal->floating->value);
      break;
    case L_BOOL:
      label = new_array(char, 23);
      snprintf(label, 23, "Boolean\\n%s", literal->boolean->value ? "true" : "false");
      break;
    case L_STRING:
      char *str = str_to_cstring(literal->string->value);
      label = new_array(char, literal->string->value.len + 19);
      snprintf(label, literal->string->value.len + 19, "String\\n'%s'", str);
      free(str);
      break;
    case L_NIL:
      label = new_array(char, 4);
      snprintf(label, 4, "Nil");
      break;
    case L_LIST:
      label = new_array(char, 5);
      snprintf(label, 5, "List");
      for (u32 i = 0; i < literal->list->len; i++) {
        dot_expr(literal->list->exprs[i], id);
      }
      break;
    case L_MAP:
      label = new_array(char, 6);
      snprintf(label, 6, "Map");
      for (u32 i = 0; i < literal->map->len; i++) {
        dot_map_entry(literal->map->entries[i], id);
      }
      break;
    case L_ENUM:
      label = new_array(char, 256);
      snprintf(label, 256, "Enum .%s", literal->enum_literal->value->lexeme);
      if (literal->enum_literal->type) dot_type(literal->enum_literal->type, id);
      break;
    case L_STRUCT:
      label = new_array(char, 7);
      snprintf(label, 7, "Struct");
      for (u32 i = 0; i < literal->struct_literal->len; i++) {
        dot_struct_literal_field(literal->struct_literal->fields[i], id);
      }
      break;
  }

  _node(id, label, NODE_BASE);
  _edge(pid, id);
  free(label);
}

static void dot_variable(VariableExpr *var, u64 pid) {
  u64 id = next_id();
  char *label;

  switch (var->type) {
    case V_NAMED:
      label = new_array(char, 256);
      snprintf(label, 256, "Variable\\n%s", var->named->name->lexeme);
      break;
    case V_STRUCT_MEMBER:
      label = new_array(char, 256);
      snprintf(label, 256, "Struct Member\\n%s", var->struct_member->name->lexeme);
      dot_variable(var->struct_member->struct_expr, id);
      break;
    case V_INDEX:
      label = new_array(char, 6);
      snprintf(label, 6, "Index");
      dot_variable(var->indexed->container, id);
      dot_expr(var->indexed->index, id);
      break;
  }

  _node(id, label, NODE_BASE);
  _edge(pid, id);
  free(label);
}

static void dot_unary(UnaryExpr *unary, u64 pid) {
  u64 id = next_id();
  _node(id, TOKEN_LEXEME[unary->op], NODE_BASE);
  _edge(pid, id);
  dot_expr(unary->expr, id);
}

static void dot_binary(BinaryExpr *binary, u64 pid) {
  u64 id = next_id();
  _node(id, TOKEN_LEXEME[binary->op], NODE_BASE);
  _edge(pid, id);
  dot_expr(binary->left, id);
  dot_expr(binary->right, id);
}

static void dot_group(GroupExpr *group, u64 pid) {
  u64 id = next_id();
  _node(id, "Group", NODE_BASE);
  _edge(pid, id);
  dot_expr(group->inner_expr, id);
}

static void dot_assignment(AssignmentExpr *assign, u64 pid) {
  u64 id = next_id();
  _node(id, "=", NODE_BASE);
  _edge(pid, id);
  dot_variable(assign->left, id);
  dot_expr(assign->right, id);
}

static void dot_if(IfExpr *if_expr, u64 pid) {
  u64 id = next_id();
  _node(id, if_expr->false_branch == NULL ? "If" : "If/Else", NODE_BASE);
  _edge(pid, id);
  dot_expr(if_expr->condition, id);
  dot_expr(if_expr->true_branch, id);
  if (if_expr->false_branch) dot_expr(if_expr->false_branch, id);
}

static void dot_for(ForExpr *for_expr, u64 pid) {
  u64 id = next_id();
  char label[256];
  if (for_expr->idx_id) {
    snprintf(label, 256, "For %s, %s In", for_expr->var_id->lexeme, for_expr->idx_id->lexeme);
  } else {
    snprintf(label, 256, "For %s In", for_expr->var_id->lexeme);
  }
  _node(id, label, NODE_BASE);
  _edge(pid, id);
  dot_expr(for_expr->iterable, id);
  dot_expr(for_expr->body, id);
}

static void dot_block(BlockExpr *block, u64 pid) {
  u64 id = next_id();
  _node(id, "Block", NODE_BASE);
  _edge(pid, id);
  for (u32 i = 0; i < block->len; i++) {
    dot_stmt(block->statements[i], id);
  }
}

static void dot_expr(Expr *expr, u64 pid) {
  switch (expr->type) {
    case EXPR_LITERAL: return dot_literal(expr->literal, pid);
    case EXPR_VARIABLE: return dot_variable(expr->variable, pid);
    case EXPR_UNARY: return dot_unary(expr->unary, pid);
    case EXPR_BINARY: return dot_binary(expr->binary, pid);
    case EXPR_GROUP: return dot_group(expr->group, pid);
    case EXPR_ASSIGNMENT: return dot_assignment(expr->assignment, pid);
    case EXPR_IF: return dot_if(expr->if_expr, pid);
    case EXPR_FOR: return dot_for(expr->for_expr, pid);
    case EXPR_BLOCK: return dot_block(expr->block, pid);
  }
}

static void dot_decl(DeclarationStmt *decl, u64 pid) {
  u64 id = next_id();
  char label[256];
  snprintf(label, 256, "Declaration\\n%s", decl->left->lexeme);
  _node(id, label, NODE_STMT);
  _edge(pid, id);
  if (decl->type) dot_type(decl->type, id);
  dot_expr(decl->right, id);
}

static void dot_alias(TypeAliasStmt *alias, u64 pid) {
  u64 id = next_id();
  char label[256];
  snprintf(label, 256, "Type alias\\n%s", alias->alias->lexeme);
  _node(id, label, NODE_STMT);
  _edge(pid, id);
  dot_type(alias->type, id);
}

static void dot_error(u64 pid) {
  u64 id = next_id();
  _node(id, "Parse ERROR", NODE_ERROR);
  _edge(pid, id);
}

static void dot_stmt(Stmt *stmt, u64 pid) {
  switch (stmt->type) {
    case STMT_EXPR: return dot_expr(stmt->expr, pid);
    case STMT_DECLARATION: return dot_decl(stmt->decl, pid);
    case STMT_TYPE_ALIAS: return dot_alias(stmt->type_alias, pid);
    case STMT_PARSE_ERROR: return dot_error(pid);
  }
}

static void dot_program(Program *program) {
  u64 id = next_id();
  _node(id, "Program", NODE_ROOT);
  for (u32 i = 0; i < program->len; i++) {
    dot_stmt(program->statements[i], id);
  }
}

void ast_generate_dot(Program *ast, FILE *out_file) {
  _of = out_file;
  _id = 0;
  _printfln("digraph AST {");
  dot_program(ast);
  _printfln("}");
  _of = NULL;
}
