#ifndef AST_DOT_HEADER
#define AST_DOT_HEADER

#include <stdio.h>

#include <ast/ast.h>

void ast_generate_dot(Program *ast, FILE *out_file);

#endif
