#ifndef FRONTEND_HEADER
#define FRONTEND_HEADER

#include <ast/ast.h>
#include <support/types.h>
#include <support/logger.h>

typedef struct {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} Location;

typedef struct {
  char *lexeme;
  u32 len;

  FlexContext ctx;
  TokenLabel label;
  SemanticValue *semantic_value;

  Location location;
} Token;

typedef struct {
  void *location;
  void *parser;
  void *scanner;
  Logger *scan_logger;
  Logger *parse_logger;
} Frontend;

void fe_init(CompilerState *compiler_state);
void fe_shutdown();

Token *fe_create_token(TokenLabel label);
void fe_free_token(Token *token);

CompilationStatus fe_push_token(Token *token);
void fe_enter_context(FlexContext ctx);
void fe_leave_context();

void fe_set_ast(Program *ast);

CompilationStatus fe_parse();

void fe_scanner_log(LogLevel level, const char *const format, ...);
void fe_parser_log(LogLevel level, const char *const format, ...);

#endif
