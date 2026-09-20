#include <stdlib.h>

#include <ast/ast.h>
#include <frontend/frontend.h>
#include <frontend/bison/bison_parser.h>
#include <support/types.h>
#include <support/logger.h>
#include <support/str.h>

#include "flex_actions.h"

CompilationStatus lex_operator(TokenLabel label) {
  Token *token = fe_create_token(label);
  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_keyword(TokenLabel label) {
  Token *token = fe_create_token(label);
  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_integer_literal() {
  Token *token = fe_create_token(INTEGER);
  token->semantic_value->integer = strtoll(token->lexeme, NULL, 10);

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_float_literal() {
  Token *token = fe_create_token(FLOAT);
  token->semantic_value->floating = strtod(token->lexeme, NULL);

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_string_literal() {
  Token *token = fe_create_token(STRING);
  str lexeme = str_from_cstring(token->lexeme);
  token->semantic_value->string = str_clone(str_slice(lexeme, 1, -1));
  str_free(lexeme);

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_boolean_literal() {
  Token *token = fe_create_token(BOOL);
  token->semantic_value->boolean = token->lexeme[0] == 't';

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_identifier() {
  Token *token = fe_create_token(IDENTIFIER);
  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_begin_multiline_comment(FlexContext ctx) {
  fe_enter_context(ctx);
  return IN_PROGRESS;
}

CompilationStatus lex_end_multiline_comment() {
  fe_leave_context();
  return IN_PROGRESS;
}

CompilationStatus lex_ignored() {
  Token *token = fe_create_token(IGNORED);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_unknown() {
  Token *token = fe_create_token(UNKNOWN);
  fe_free_token(token);
  return FAILED;
}

CompilationStatus lex_eof() {
  Token *token = fe_create_token(END);
  fe_push_token(token);
  fe_free_token(token);
  return SUCCEEDED;
}
