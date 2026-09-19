#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "frontend.h"

#include <ast/ast.h>
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

static const char *token_label_str[] = {
  [PAREN_L] = "Paren_L",
  [PAREN_R] = "Paren_R",
  [SQUARE_L] = "Square_L",
  [SQUARE_R] = "Square_R",
  [CURLY_L] = "Curly_L",
  [CURLY_R] = "Curly_R",
  [COLON] = "Colon",
  [COMMA] = "Comma",
  [DOT] = "Dot",
  [MINUS] = "Minus",
  [PLUS] = "Plus",
  [STAR] = "Star",
  [SLASH] = "Slash",
  [QUESTION_MARK] = "Question_Mark",
  [HASH] = "Hash",
  [EQUAL] = "Equal",
  [EQUAL_EQUAL] = "Equal_Equal",
  [BANG] = "Bang",
  [BANG_EQUAL] = "Bang_Equal",
  [GREATER] = "Greater",
  [GREATER_EQUAL] = "Greater_Equal",
  [LESS] = "Less",
  [LESS_EQUAL] = "Less_Equal",
  [IF] = "If",
  [ELSE] = "Else",
  [FOR] = "For",
  [IN] = "In",
  [NIL] = "Nil",
  [IS] = "Is",
  [OF] = "Of",
  [STRUCT] = "Struct",
  [UNION] = "Union",
  [ENUM] = "Enum",
  [FUNCTION] = "Function",
  [IMPORT] = "Import",
  [INTEGER] = "Integer Literal",
  [FLOAT] = "Float Literal",
  [STRING] = "String Literal",
  [BOOL] = "Boolean Literal",
  [IDENTIFIER] = "Identifier",
  [IGNORED] = "Ignored Lexeme",
  [UNKNOWN] = "Unknown Lexeme",
  [END] = "EOF",
};

static void _log_token(Token *token, LogLevel level) {
  if (token->label == END) {
    fe_log(level, "%s @ %u:%u",
      token_label_str[token->label],
      token->location.first_line,
      token->location.first_column
    );
    return;
  }

  fe_log(level, "%s '%s' @ %u:%u-%u:%u",
    token_label_str[token->label],
    token->lexeme,
    token->location.first_line,
    token->location.first_column,
    token->location.last_line,
    token->location.last_column
  );
}

Token *fe_create_token(TokenLabel label) {
  Token *token = new (Token);
  token->label = label;
  token->ctx = flex_current_context(f);
  token->location = *((Location *)yyget_lloc(f->scanner));
  token->semantic_value = new(SemanticValue);
  token->len = yyget_leng(f->scanner);
  token->lexeme = strndup(yyget_text(f->scanner), token->len);

  if (label != IGNORED) _log_token(token, label == UNKNOWN ? LOG_FATAL : LOG_DEBUG);
  return token;
}

void fe_free_token(Token *token) {
  if (!token) return;

  if (token->lexeme) free(token->lexeme);
  if (token->semantic_value) free(token->semantic_value);
  free(token);
}

CompilationStatus fe_push_token(Token *token) {
  return yypush_parse(f->parser, token->label, token->semantic_value, f->location);
}

void fe_enter_context(FlexContext ctx) {
  flex_enter_context(f, ctx);
}

void fe_leave_context() {
  flex_leave_context(f);
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
