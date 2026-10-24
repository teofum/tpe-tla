#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <ast/ast.h>
#include <support/state.h>

typedef enum {
  // Parser errors
  ERR_SYNTAX,           // Syntax error
  ERR_IMPORT,           // Invalid import type
  // Static analysis errors
  ERR_REDEF,            // Symbol redefinition
  ERR_UNDEF,            // Undefined symbol reference
  // Type errors
  ERR_TYPE,             // Type error
} ErrorType;

typedef enum {
  ERR_ERROR,
  ERR_WARNING,
} ErrorLevel;

typedef enum {
  CS_PARSE,
  CS_STATIC_ANALYSIS,
  CS_TYPECHECK,
  CS_EVAL,
  CS_CODEGEN,
} CompilationStage;

typedef struct SyntaxError SyntaxError;
typedef struct ImportError ImportError;
typedef struct RedefError RedefError;
typedef struct UndefError UndefError;
typedef struct TypeError TypeError;

typedef struct {
  ErrorType type;
  ErrorLevel level;
  CompilationStage stage;
  Location loc;
  const char *filepath;

  union {
    SyntaxError *syntax;
    ImportError *import_type;
    RedefError *redefinition;
    UndefError *undef_symbol;
    TypeError *type_error;
  };
} Error;

struct SyntaxError {
  u32 expected_len;
  char **expected;
  char *found;
};

struct ImportError {
  char *type;
};

struct RedefError {
  const char *object_type;
  char *object_name;
  Location previous_definition;
};

struct UndefError {
  const char *object_type;
  char *object_name;
};

struct TypeError {
  char *message;
};

void err_init(CompilerState *compiler_state);
void err_shutdown();

void err_report_syntax(SyntaxError *error, const Location *location, ErrorLevel level);
void err_report_import(ImportError *error, const Location *location, ErrorLevel level);
void err_report_redef(RedefError *error, const Location *location, ErrorLevel level);
void err_report_undef(UndefError *error, const Location *location, ErrorLevel level);
void err_report_type(TypeError *error, const Location *location, ErrorLevel level);
bool err_gate();

void err_free_error(Error *e);
void err_free_syntax_error(SyntaxError *e);
void err_free_import_error(ImportError *e);
void err_free_redef_error(RedefError *e);
void err_free_undef_error(UndefError *e);
void err_free_type_error(TypeError *e);

#endif
