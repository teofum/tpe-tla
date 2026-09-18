#ifndef TOKEN_HEADER
#define TOKEN_HEADER

#include <support/types.h>

typedef i32 TokenLabel;

typedef struct {
  char *lexeme;
  u32 len;

  FlexContext ctx;
  TokenLabel label;

  u32 line;
  u32 col;
} Token;

#endif
