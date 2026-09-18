#include <stdlib.h>

// Includes so clangd doesn't bitch about missing types in the bison header
#include <frontend/token.h>
#include <support/types.h>

#include <frontend/bison/bison_parser.h>
#include <frontend/flex/flex_scanner.h>
#include <support/util.h>

#include "lexer.h"

LexicalAnalyzer *la_create() {
  LexicalAnalyzer *la = new (LexicalAnalyzer);
  yylex_init(&la->scanner);
  la->parser = yypstate_new();
  la->location = new (YYLTYPE);

  flex_enter_context(la, 0);
  return la;
}

void la_free(LexicalAnalyzer *la) {
  if (!la)
    return;

  if (la->scanner)
    yylex_destroy(la->scanner);
  if (la->parser)
    yypstate_delete(la->parser);
  if (la->location)
    free(la->location);

  free(la);
}

FlexContext la_ctx(LexicalAnalyzer *la) { return flex_current_context(la); }
