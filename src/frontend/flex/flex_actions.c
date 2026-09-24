#include <stdlib.h>
#include <string.h>

#include <ast/ast.h>
#include <frontend/frontend.h>
#include <frontend/bison/bison_parser.h>
#include <support/types.h>
#include <support/logger.h>
#include <support/str.h>
#include <support/util.h>

#include "flex_actions.h"

static TokenMeta *_token_meta(Token *token) {
  TokenMeta *meta = new(TokenMeta);
  *meta = (TokenMeta){
    .label = token->label,
    .len = token->len,
    .lexeme = strndup(token->lexeme, token->len),
  };
  return meta;
}

CompilationStatus lex_operator(TokenLabel label) {
  Token *token = fe_create_token(label);
  token->semantic_value->token = token->label;

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_keyword(TokenLabel label) {
  Token *token = fe_create_token(label);
  token->semantic_value->token = token->label;

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_integer_literal() {
  Token *token = fe_create_token(INTEGER);
  IntegerLiteral *l = new(IntegerLiteral);
  *l = (IntegerLiteral){
    .value = strtoll(token->lexeme, NULL, 10),
    .meta = _token_meta(token),
  };
  token->semantic_value->integer = l;

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_float_literal() {
  Token *token = fe_create_token(FLOAT);
  FloatLiteral *l = new(FloatLiteral);
  *l = (FloatLiteral){
    .value = strtod(token->lexeme, NULL),
    .meta = _token_meta(token),
  };
  token->semantic_value->floating = l;

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_string_literal() {
  Token *token = fe_create_token(STRING);
  str lexeme = str_from_cstring(token->lexeme);
  StringLiteral *l = new(StringLiteral);
  *l = (StringLiteral){
    .value = str_clone(str_slice(lexeme, 1, -1)),
    .meta = _token_meta(token),
  };
  str_free(lexeme);
  token->semantic_value->string = l;

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_boolean_literal() {
  Token *token = fe_create_token(BOOL);
  BooleanLiteral *l = new(BooleanLiteral);
  *l = (BooleanLiteral){
    .value = token->lexeme[0] == 't',
    .meta = _token_meta(token),
  };
  token->semantic_value->boolean = l;

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_identifier() {
  Token *token = fe_create_token(IDENTIFIER);
  token->semantic_value->identifier = _token_meta(token);

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_begin_multiline_comment(FlexContext ctx) {
  fe_enter_context(ctx);
  return IN_PROGRESS;
}

CompilationStatus lex_end_multiline_comment() {
  fe_leave_context();
  return IN_PROGRESS;
}

CompilationStatus lex_newline() {
  Token *token = fe_create_token(NL);
  token->semantic_value->token = token->label;

  fe_push_token(token);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_ignored() {
  Token *token = fe_create_token(IGNORED);
  fe_free_token(token);
  return IN_PROGRESS;
}

CompilationStatus lex_unknown() {
  Token *token = fe_create_token(UNKNOWN);
  fe_free_token(token);
  return FAILED;
}

CompilationStatus lex_eof() {
  Token *token = fe_create_token(END);
  token->semantic_value->token = token->label;

  fe_push_token(token);
  fe_free_token(token);
  return SUCCEEDED;
}
