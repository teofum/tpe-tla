#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <ast/ast.h>

typedef enum {
  ERR_SYNTAX,
} ErrorType;

typedef struct {
  Location loc;
} SyntaxError;

typedef struct {
  ErrorType type;
  union {
    SyntaxError syntax;
  };
} Error;



#endif
