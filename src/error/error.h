#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <ast/ast.h>

typedef enum {
  ERR_SYNTAX,
} ErrorType;

typedef struct SyntaxError SyntaxError;

typedef struct {
  ErrorType type;
  union {
    SyntaxError *syntax;
  };
} Error;

struct SyntaxError {
  u32 expected_len;
  char **expected;
  char *found;
  Location loc;
};

void err_init();
void err_shutdown();

void err_report_syntax(SyntaxError *error);
bool err_gate();

void err_free_error(Error *e);
void err_free_syntax_error(SyntaxError *e);

#endif
