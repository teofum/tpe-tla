#include <stdlib.h>

#include <frontend/flex/flex_scanner.h>
#include <support/types.h>
#include <support/util.h>

#include "lexer.h"
#include "token.h"

Token *tok_create(LexicalAnalyzer *la, TokenLabel label) {
  Token *token = new (Token);
  token->label = label;
  token->ctx = flex_current_context(la);
  // token->len = yyget_len
  token->lexeme = malloc(token->len + 1);

  return token;
}
