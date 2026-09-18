#include <frontend/frontend.h>
#include <frontend/bison/bison_parser.h>
#include <support/types.h>
#include <support/logger.h>

#include "flex_actions.h"

CompilationStatus lex_integer_literal() {
  Token *token = fe_create_token(INTEGER);
  fe_log(LOG_DEBUG, "Integer '%s'", token->lexeme);
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
