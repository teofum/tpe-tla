#ifndef AST_DOT_HEADER
#define AST_DOT_HEADER

#include <stdio.h>

#include <ast/ast.h>
#include <support/state.h>

void ast_generate_dot(CompilerState *cs, u32 ast_idx);

char *ast_dot_filepath(CompilerState *cs, const char *input_filepath);

#endif
