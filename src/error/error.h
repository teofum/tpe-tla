#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <ast/ast.h>
#include <support/state.h>

typedef enum {
  // Parser errors
  ERR_SYNTAX,           // Syntax error
  ERR_STRUCT_FIELD,     // Duplicate struct field name
  ERR_IMPORT,           // Invalid import type
  ERR_TYPED_IMPORT,     // Typed import with an incompatible import type
  ERR_IMPORT_FILE,      // Imported file doesn't exist
  ERR_IMPORT_CSV,       // CSV import error
  // Static analysis errors
  ERR_REDEF,            // Symbol redefinition
  ERR_UNDEF,            // Undefined symbol reference
  ERR_ASSIGNMENT,       // Assignment to invalid lhs expression
  ERR_FN_ARG_COUNT,     // Function call argument count mismatch
  ERR_FN_ARG_NAME,      // Function call argument invalid name
  ERR_FN_ARG_POS,       // Function call positional argument after named
  ERR_FN_PARAM_REQ,     // Function call required parameter unset
  // Type errors
  ERR_TYPE,             // Type error
  ERR_COMPOSE,          // Compose expression in function with no compose type
  // Evaluation errors
  ERR_OOB,              // Index out of bounds
  ERR_RANGE,            // Invalid range
  ERR_RANGE_PARTIAL,    // Partial range where full range is required
  ERR_EVAL,             // Generic evaluation error
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
typedef struct StructFieldError StructFieldError;
typedef struct ImportError ImportError;
typedef struct ImportFileError ImportFileError;
typedef struct ImportCSVError ImportCSVError;
typedef struct RedefError RedefError;
typedef struct UndefError UndefError;
typedef struct TypeError TypeError;
typedef struct FnArgCountError FnArgCountError;
typedef struct FnArgNameError FnArgNameError;
typedef struct FnArgPosError FnArgPosError;
typedef struct FnParamReqError FnParamReqError;
typedef struct BoundsError BoundsError;
typedef struct RangeError RangeError;
typedef struct EvalError EvalError;

typedef struct {
  ErrorType type;
  ErrorLevel level;
  CompilationStage stage;
  Location loc;
  const char *filepath;

  union {
    SyntaxError *syntax;
    StructFieldError *struct_field;
    ImportError *import_type;
    ImportFileError *import_file;
    ImportCSVError *import_csv;
    RedefError *redefinition;
    UndefError *undef_symbol;
    TypeError *type_error;
    FnArgCountError *fn_arg_count;
    FnArgNameError *fn_arg_name;
    FnArgPosError *fn_arg_pos;
    FnParamReqError *fn_param_req;
    BoundsError *bounds;
    RangeError *range;
    EvalError *key;
  };
} Error;

struct SyntaxError {
  u32 expected_len;
  char **expected;
  char *found;
};

struct StructFieldError {
  char *name;
  Location previous_definition;
};

struct ImportError {
  char *type;
};

struct ImportFileError {
  char *path;
};

struct ImportCSVError {
  char *message;
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

struct FnArgCountError {
  u32 expected;
  u32 actual;
};

struct FnArgNameError {
  const FunctionCallExpr *call;
  const Argument *arg;
};

struct FnArgPosError {
  const Argument *arg;
};

struct FnParamReqError {
  const FunctionCallExpr *call;
  const Parameter *param;
};

struct BoundsError {
  i64 idx;
  i64 array_len;
};

struct RangeError {
  i64 start;
  i64 end;
};

struct EvalError {
  char *message;
};

void err_init(CompilerState *compiler_state);
void err_shutdown();

void err_report_syntax(SyntaxError *error, const Location *loc, ErrorLevel level);
void err_report_struct_field(char *name, const Location *prev_loc, const Location *loc, ErrorLevel level);
void err_report_import(ImportError *error, const Location *loc, ErrorLevel level);
void err_report_typed_import(ImportError *error, const Location *loc, ErrorLevel level);
void err_report_import_file(char *path, const Location *loc, ErrorLevel level);
void err_report_import_csv(char *message, const Location *loc, ErrorLevel level);
void err_report_redef(RedefError *error, const Location *loc, ErrorLevel level);
void err_report_undef(UndefError *error, const Location *loc, ErrorLevel level);
void err_report_type(TypeError *error, const Location *loc, ErrorLevel level);
void err_report_fn_arg_count(FnArgCountError *error, const Location *loc, ErrorLevel level);
void err_report_fn_arg_name(const FunctionCallExpr *call, const Argument *arg, ErrorLevel level);
void err_report_fn_arg_pos(const Argument *arg, ErrorLevel level);
void err_report_fn_param_req(const FunctionCallExpr *call, const Parameter *param, ErrorLevel level);
void err_report_compose(const Location *loc, ErrorLevel level);
void err_report_oob(i64 idx, i64 array_len, const Location *loc, ErrorLevel level);
void err_report_range(i64 start, i64 end, const Location *loc, ErrorLevel level);
void err_report_partial_range(const Location *loc, ErrorLevel level);
void err_report_eval(char *message, const Location *loc, ErrorLevel level);
void err_report_assignment(const Location *loc, ErrorLevel level);
bool err_gate();

void err_free_error(Error *e);
void err_free_syntax_error(SyntaxError *e);
void err_free_struct_field_error(StructFieldError *e);
void err_free_import_error(ImportError *e);
void err_free_import_file_error(ImportFileError *e);
void err_free_import_csv_error(ImportCSVError *e);
void err_free_redef_error(RedefError *e);
void err_free_undef_error(UndefError *e);
void err_free_type_error(TypeError *e);
void err_free_fn_arg_count_error(FnArgCountError *e);
void err_free_fn_arg_name_error(FnArgNameError *e);
void err_free_fn_arg_pos_error(FnArgPosError *e);
void err_free_fn_param_req_error(FnParamReqError *e);
void err_free_bounds_error(BoundsError *e);
void err_free_range_error(RangeError *e);
void err_free_eval_error(EvalError *e);

#endif
