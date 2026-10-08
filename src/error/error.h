#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <ast/ast.h>
#include <support/state.h>

typedef enum {
  ERR_SYNTAX,
  ERR_IMPORT_TYPE,
  ERR_REDEFINITION,
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
typedef struct RedefinitionError RedefinitionError;

typedef struct {
  ErrorType type;
  ErrorLevel level;
  CompilationStage stage;
  Location loc;
  const char *filepath;

  union {
    SyntaxError *syntax;
    ImportTypeError *import_type;
    RedefinitionError *redefinition;
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

struct RedefinitionError {
  const char *object_type;
  char *object_name;
  Location previous_definition;
};

void err_init(CompilerState *compiler_state);
void err_shutdown();

void err_report_syntax(SyntaxError *error, Location *location, ErrorLevel level);
void err_report_import_type(ImportTypeError *error, Location *location, ErrorLevel level);
void err_report_redefinition(RedefinitionError *error, Location *location, ErrorLevel level);
bool err_gate();

void err_free_error(Error *e);
void err_free_syntax_error(SyntaxError *e);
void err_free_import_type_error(ImportTypeError *e);
void err_free_redefinition_error(RedefinitionError *e);

#endif
