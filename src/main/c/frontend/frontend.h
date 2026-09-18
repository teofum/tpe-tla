#ifndef FRONTEND_HEADER
#define FRONTEND_HEADER

#include <support/types.h>
#include <support/logger.h>

typedef i32 TokenLabel;

typedef struct {
  char *lexeme;
  u32 len;

  FlexContext ctx;
  TokenLabel label;
  SemanticValue *semantic_value;

  u32 line;
  u32 col;
} Token;

typedef struct {
  void *location;
  void *parser;
  void *scanner;
  Logger *logger;
} Frontend;

void fe_init();
void fe_shutdown();

Token *fe_create_token(TokenLabel label);
void fe_free_token(Token *token);

CompilationStatus fe_parse();

void fe_log(LogLevel level, const char *const format, ...);

#endif
