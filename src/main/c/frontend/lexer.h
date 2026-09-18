#ifndef LEXER_HEADER
#define LEXER_HEADER

#include <support/types.h>

typedef struct {
  void *location;
  void *parser;
  void *scanner;
} LexicalAnalyzer;

LexicalAnalyzer *la_create();
void la_free(LexicalAnalyzer *la);
FlexContext la_ctx(LexicalAnalyzer *la);

#endif
