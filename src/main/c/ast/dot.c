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

static void _node(u64 id, const char *label) {
  _printfln("%llu [shape=box,label=\"%s\"];", id, label);
}

static void _edge(u64 id1, u64 id2) {
  _printfln("%llu -> %llu;", id1, id2);
}

static void dot_expr(Expr *expr, u64 pid);

static void dot_literal(LiteralExpr *literal, u64 pid) {
  u64 id = next_id();
  char *label;

  switch (literal->type) {
    case L_INTEGER:
      label = new_array(char, 256);
      snprintf(label, 256, "Integer Literal\\n%lld", (long long)literal->integer->value);
      break;
    case L_FLOAT:
      label = new_array(char, 256);
      snprintf(label, 256, "Float Literal\\n%f", literal->floating->value);
      break;
    case L_BOOL:
      label = new_array(char, 23);
      snprintf(label, 23, "Boolean Literal\\n%s", literal->boolean->value ? "true" : "false");
      break;
    case L_STRING:
      char *str = str_to_cstring(literal->string->value);
      label = new_array(char, literal->string->value.len + 19);
      snprintf(label, literal->string->value.len + 19, "String Literal\\n'%s'", str);
      free(str);
      break;
  }

  _node(id, label);
  _edge(pid, id);
  free(label);
}

static void dot_variable(VariableExpr *var, u64 pid) {
  u64 id = next_id();
  char *label;

  switch (var->type) {
    case V_NAMED:
      label = new_array(char, 256);
      snprintf(label, 256, "Variable\\n%s", var->named->meta->lexeme);
      break;
    case V_STRUCT_MEMBER:
      label = new_array(char, 256);
      snprintf(label, 256, "Struct Member\\n%s", var->struct_member->meta->lexeme);
      dot_variable(var->struct_member->struct_expr, id);
      break;
    case V_INDEX:
      label = new_array(char, 6);
      snprintf(label, 6, "Index");
      dot_variable(var->indexed->container, id);
      dot_expr(var->indexed->index, id);
      break;
  }

  _node(id, label);
  _edge(pid, id);
  free(label);
}

static void dot_unary(UnaryExpr *unary, u64 pid) {
  u64 id = next_id();
  _node(id, unary->op->lexeme);
  _edge(pid, id);
  dot_expr(unary->expr, id);
}

static void dot_binary(BinaryExpr *binary, u64 pid) {
  u64 id = next_id();
  _node(id, binary->op->lexeme);
  _edge(pid, id);
  dot_expr(binary->left, id);
  dot_expr(binary->right, id);
}

static void dot_group(GroupExpr *group, u64 pid) {
  u64 id = next_id();
  _node(id, "Group");
  _edge(pid, id);
  dot_expr(group->inner_expr, id);
}

static void dot_assignment(AssignmentExpr *assign, u64 pid) {
  u64 id = next_id();
  _node(id, "=");
  _edge(pid, id);
  dot_variable(assign->left, id);
  dot_expr(assign->right, id);
}

static void dot_type(Type *type, u64 pid) {
  u64 id = next_id();
  char *label;

  switch (type->type) {
    case T_NAMED:
      label = new_array(char, 256);
      snprintf(label, 256, "Type\\n%s", type->named->meta->lexeme);
      break;
  }

  _node(id, label);
  _edge(pid, id);
  free(label);
}

static void dot_decl(DeclarationExpr *decl, u64 pid) {
  u64 id = next_id();
  char label[256];
  snprintf(label, 256, "Declaration\\n%s", decl->left->lexeme);
  _node(id, label);
  _edge(pid, id);
  if (decl->type) dot_type(decl->type, id);
  dot_expr(decl->right, id);
}

static void dot_block(BlockExpr *block, u64 pid) {
  u64 id = next_id();
  _node(id, "Block");
  _edge(pid, id);
  for (u32 i = 0; i < block->len; i++) {
    dot_expr(block->exprs[i], id);
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
    case EXPR_DECLARATION: return dot_decl(expr->declaration, pid);
    case EXPR_BLOCK: return dot_block(expr->block, pid);
  }
}

static void dot_program(Program *program) {
  u64 id = next_id();
  _node(id, "Program");
  for (u32 i = 0; i < program->len; i++) {
    dot_expr(program->exprs[i], id);
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
