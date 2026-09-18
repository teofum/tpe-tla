#include <frontend/frontend.h>
#include <frontend/bison/bison_parser.h>
#include <support/types.h>
#include <support/logger.h>

#include "flex_actions.h"

CompilationStatus lex_nil() {
  Token *token = fe_create_token(NIL);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_integer_literal() {
  Token *token = fe_create_token(INTEGER);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_float_literal() {
  Token *token = fe_create_token(FLOAT);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_string_literal() {
  Token *token = fe_create_token(STRING);
  fe_free_token(token);
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
  // TODO: create EOF token
  return SUCCEEDED;
}
