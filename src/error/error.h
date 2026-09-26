#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <ast/ast.h>

typedef enum {
  ERR_SYNTAX,
  ERR_IMPORT_TYPE,
} ErrorType;

typedef enum {
  ERR_ERROR,
  ERR_WARNING,
} ErrorLevel;

typedef enum {
  CS_PARSE,
} CompilationStage;

typedef struct SyntaxError SyntaxError;
typedef struct ImportTypeError ImportTypeError;

typedef struct {
  ErrorType type;
  ErrorLevel level;
  CompilationStage stage;
  Location loc;

  union {
    SyntaxError *syntax;
    ImportTypeError *import_type;
  };
} Error;

struct SyntaxError {
  u32 expected_len;
  char **expected;
  char *found;
};

struct ImportTypeError {
  char *type;
};

void err_init();
void err_shutdown();

void err_report_syntax(SyntaxError *error, Location *location, ErrorLevel level);
void err_report_import_type(ImportTypeError *error, Location *location, ErrorLevel level);
bool err_gate();

void err_free_error(Error *e);
void err_free_syntax_error(SyntaxError *e);
void err_free_import_type_error(ImportTypeError *e);

#endif
