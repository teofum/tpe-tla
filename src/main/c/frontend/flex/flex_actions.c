#include <frontend/lexer.h>
#include <support/types.h>

#include "flex_actions.h"

static LexicalAnalyzer *la = NULL;

CompilationStatus lex_integer_literal() {
  // create token
  return IN_PROGRESS;
}
