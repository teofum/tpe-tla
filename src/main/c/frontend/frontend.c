#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "frontend.h"

#include <frontend/bison/bison_parser.h>
#include <frontend/flex/flex_scanner.h>
#include <support/types.h>
#include <support/util.h>
#include <support/logger.h>

static Frontend *f = NULL;

void fe_init() {
  f = new (Frontend);
  yylex_init(&f->scanner);
  f->parser = yypstate_new();
  f->location = new (YYLTYPE);
  f->logger = logger_create("Frontend", stderr);

  flex_enter_context(f, 0);
}


void fe_shutdown() {
  if (!f) return;

  if (f->scanner) yylex_destroy(f->scanner);
  if (f->parser) yypstate_delete(f->parser);
  if (f->location) free(f->location);
  if (f->logger) logger_free(f->logger);
  free(f);

  f = NULL;
}

Token *fe_create_token(TokenLabel label) {
  Token *token = new (Token);
  token->label = label;
  token->ctx = flex_current_context(f);
  token->len = yyget_leng(f->scanner);
  token->line = yyget_lineno(f->scanner);
  token->col = yyget_column(f->scanner);
  token->lexeme = strndup(yyget_text(f->scanner), token->len);
  token->semantic_value = new(SemanticValue);

  return token;
}

void fe_free_token(Token *token) {
  if (!token) return;

  if (token->lexeme) free(token->lexeme);
  if (token->semantic_value) free(token->semantic_value);
  free(token);
}

static FlexContext _ctx() {
  return flex_current_context(f);
}

static CompilationStatus _next() {
  return yylex(NULL, f->location, f->scanner);
}

CompilationStatus fe_parse() {
  CompilationStatus status = IN_PROGRESS;
  while (status == IN_PROGRESS) status = _next();

  return status;
}

void fe_log(LogLevel level, const char *const format, ...) {
  va_list args;
  va_start(args, format);
  logger_logv(f->logger, level, format, args);
  va_end(args);
}
