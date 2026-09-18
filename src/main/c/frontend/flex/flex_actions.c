#include <frontend/frontend.h>
#include <frontend/bison/bison_parser.h>
#include <support/types.h>
#include <support/logger.h>

#include "flex_actions.h"

CompilationStatus lex_nil() {
  Token *token = fe_create_token(NIL);
  fe_log(LOG_DEBUG, "Nil", token->lexeme);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_integer_literal() {
  Token *token = fe_create_token(INTEGER);
  fe_log(LOG_DEBUG, "Integer '%s'", token->lexeme);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_float_literal() {
  Token *token = fe_create_token(FLOAT);
  fe_log(LOG_DEBUG, "Float '%s'", token->lexeme);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_string_literal() {
  Token *token = fe_create_token(STRING);
  fe_log(LOG_DEBUG, "String %s", token->lexeme);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_ignored() {
  Token *token = fe_create_token(IGNORED);
  fe_log(LOG_DEBUG, "Ignored '%s'", token->lexeme);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_unknown() {
  fe_log(LOG_FATAL, "Encountered unknown lexeme");
  return FAILED;
}

CompilationStatus lex_eof() {
  // TODO: create EOF token
  return SUCCEEDED;
}
