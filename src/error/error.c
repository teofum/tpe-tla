#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <support/dyn_array.h>
#include <support/logger.h>
#include <support/state.h>
#include <support/str.h>
#include <support/util.h>

#include "error.h"
#include "ast/ast.h"
#include "support/types.h"

#define FMT_LOCATION "In %s:%u:%u: "
#define INDENTED_NL "\n\t\t"
#define INDENT "\t\t"

typedef dyn_array(Error) ErrorList;

typedef struct {
  ErrorList *errors;
  Logger *logger;
} ErrorState;

static ErrorState *e;
static CompilerState *cs = NULL;

static LogLevel _error_to_log_level[] = {
  [ERR_ERROR] = LOG_ERROR,
  [ERR_WARNING] = LOG_WARNING,
};

void err_init(CompilerState *compiler_state) {
  cs = compiler_state;

  e = new(ErrorState);
  e->errors = new(ErrorList);
  da_init(e->errors);
  da_manage(e->errors, err_free_error);
  e->logger = cs->logger;
}

void err_shutdown() {
  da_free(e->errors);
  free(e->errors);
  free(e);

  e = NULL;
  cs = NULL;
}

static Error _make_error(
  ErrorType type,
  CompilationStage stage,
  ErrorLevel level,
  const Location *loc
) {
  return (Error){
    .type = type,
    .stage = stage,
    .level = level,
    .loc = *loc,
    .filepath = state_current_filepath(cs),
  };
}

void err_report_syntax(SyntaxError *s, const Location *loc, ErrorLevel level) {
  Error error = _make_error(ERR_SYNTAX, CS_PARSE, level, loc);
  error.syntax = s;
  da_push(e->errors, &error);
}

void err_report_import(ImportError *i, const Location *loc, ErrorLevel level) {
  Error error = _make_error(ERR_IMPORT, CS_PARSE, level, loc);
  error.import_type = i;
  da_push(e->errors, &error);
}

void err_report_redef(RedefError *r, const Location *loc, ErrorLevel level) {
  Error error = _make_error(ERR_REDEF, CS_STATIC_ANALYSIS, level, loc);
  error.redefinition = r;
  da_push(e->errors, &error);
}

void err_report_undef(UndefError *u, const Location *loc, ErrorLevel level) {
  Error error = _make_error(ERR_UNDEF, CS_STATIC_ANALYSIS, level, loc);
  error.undef_symbol = u;
  da_push(e->errors, &error);
}

void err_report_type(TypeError *t, const Location *loc, ErrorLevel level) {
  Error error = _make_error(ERR_TYPE, CS_TYPECHECK, level, loc);
  error.type_error = t;
  da_push(e->errors, &error);
}

void err_report_fn_arg_count(FnArgCountError *f, const Location *loc, ErrorLevel level) {
  Error error = _make_error(ERR_FN_ARG_COUNT, CS_TYPECHECK, level, loc);
  error.fn_arg_count = f;
  da_push(e->errors, &error);
}

void err_report_fn_arg_name(const FunctionCallExpr *call, const Argument *arg, ErrorLevel level) {
  Error error = _make_error(ERR_FN_ARG_NAME, CS_TYPECHECK, level, &arg->meta.loc);
  error.fn_arg_name = new(FnArgNameError);
  error.fn_arg_name->arg = arg;
  error.fn_arg_name->call = call;
  da_push(e->errors, &error);
}

void err_report_fn_arg_pos(const Argument *arg, ErrorLevel level) {
  Error error = _make_error(ERR_FN_ARG_POS, CS_TYPECHECK, level, &arg->meta.loc);
  error.fn_arg_pos = new(FnArgPosError);
  error.fn_arg_pos->arg = arg;
  da_push(e->errors, &error);
}

void err_report_fn_param_req(const FunctionCallExpr *call, const Parameter *p, ErrorLevel level) {
  Error error = _make_error(ERR_FN_PARAM_REQ, CS_TYPECHECK, level, &p->meta.loc);
  error.fn_param_req = new(FnParamReqError);
  error.fn_param_req->param = p;
  error.fn_param_req->call = call;
  da_push(e->errors, &error);
}

void err_report_compose(const Location *loc, ErrorLevel level) {
  Error error = _make_error(ERR_COMPOSE, CS_TYPECHECK, level, loc);
  da_push(e->errors, &error);
}

static void _log(Error *error, const char *const format, ...) {
  va_list args;
  va_start(args, format);
  logger_logv(e->logger, _error_to_log_level[error->level], format, args);
  va_end(args);
}

static str _error_loc(str error_str, const char *fp, Location *loc, const char *color) {
  str with_loc = str_format(
    FMT_LOCATION "%.*s",
    fp, loc->first_line, loc->first_column,
    error_str.len, error_str.ptr
  );
  str_free(error_str);

  FILE *file = fopen(fp, "r");
  if (file) {
    u32 fl = loc->first_line - 1, fc_fl = loc->first_column - 1;
    u32 ll = loc->last_line - 1, lc_ll = loc->last_column - 1;
    u32 n_lines = ll - fl + 1;
    char buf[256];
    for (u32 line = 0; line < fl; line++) {
      fgets(buf, 256, file);
    }
    for (u32 line = fl; line <= ll; line++) {
      fgets(buf, 256, file);
      u32 fc = (line == fl) ? fc_fl : 0;
      u32 lc = (line == ll) ? lc_ll : strlen(buf);
      if (lc == 0) continue;
      if (n_lines > 5 && line > fl + 1 && line < ll - 1) {
        if (line == fl + 2) {
          str_append_temp(&with_loc,
            str_format(INDENTED_NL " ...| (%u more lines)", n_lines - 4)
          );
        }
        continue;
      }

      str_append_temp(&with_loc,
        str_format(INDENTED_NL "%4u| %.*s" "%s" "%.*s" RESET "%s",
          line + 1, fc, buf, color, lc - fc, buf + fc, buf + lc
        )
      );
      for (u32 i = 0; i < lc; i++) {
        buf[i] = i < fc ? ' ' : i == fc ? '^' : '~';
      }
      buf[lc] = 0;
      str_append_temp(&with_loc,
        str_format(INDENT "    | %s" "%s" RESET, color, buf)
      );
    }
    fclose(file);
  }

  return with_loc;
}

static void _log_error_with_loc(str error_str, Error *error) {
  str with_loc = _error_loc(error_str, error->filepath, &error->loc, R);
  _log(error, "%.*s", with_loc.len, with_loc.ptr);
  str_free(with_loc);
}

static void _log_syntax_error(Error *error) {
  SyntaxError *s = error->syntax;

  str error_str = str_from_cstring("syntax error: ");
  if (s->found) {
    str_append_temp(&error_str, str_format("unexpected %s", s->found));
    if (s->expected) str_append_c(&error_str, "; ");
  }
  if (s->expected) {
    str expected = str_format("expected %s", s->expected[0]);
    for (u32 i = 1; i < s->expected_len; i++) {
      str_append_temp(&expected, str_format(", %s", s->expected[i]));
    }
    str_append_temp(&error_str, expected);
  }
  _log_error_with_loc(error_str, error);
}

static void _log_import_type_error(Error *error) {
  ImportError *it = error->import_type;

  str accepted = str_format("'%s'", import_type_str[0]);
  for (u32 i = 1; i < IMPORT_TYPE_COUNT; i++) {
    str_append_temp(&accepted, str_format(", '%s'", import_type_str[i]));
  }

  _log(error,
    FMT_LOCATION "unrecognized import type '%s'"
      INDENTED_NL "Accepted import types are: %.*s",
    error->filepath, error->loc.first_line, error->loc.first_column,
    it->type, accepted.len, accepted.ptr
  );
  str_free(accepted);
}

static void _log_redefinition_error(Error *error) {
  RedefError *r = error->redefinition;

  str error_str = str_format(
    "redefinition of %s '%s':",
    r->object_type, r->object_name
  );
  _log_error_with_loc(error_str, error);

  str prev_str = str_format("'%s' previously defined here:", r->object_name);
  str prev_with_loc = _error_loc(prev_str, error->filepath, &r->previous_definition, B);

  logger_log(e->logger, LOG_INFO, "%.*s", prev_with_loc.len, prev_with_loc.ptr);
  str_free(prev_with_loc);
}

static void _log_undef_symbol_error(Error *error) {
  UndefError *u = error->undef_symbol;

  str error_str = str_format(
    "%s '%s' is undefined",
    u->object_type, u->object_name
  );
  _log_error_with_loc(error_str, error);
}

static void _log_type_error(Error *error) {
  TypeError *t = error->type_error;

  str error_str = str_format("type error: %s", t->message);
  _log_error_with_loc(error_str, error);
}

static void _log_fn_arg_count_error(Error *error) {
  FnArgCountError *f = error->fn_arg_count;

  str error_str = str_format(
    "function argument count mismatch: expected %u, got %u",
    f->expected, f->actual
  );
  _log_error_with_loc(error_str, error);
}

static void _log_fn_arg_name_error(Error *error) {
  FnArgNameError *f = error->fn_arg_name;

  str error_str = str_format(
    "function '%s' has no parameter '%s'",
    f->call->name->lexeme, f->arg->name->lexeme
  );
  _log_error_with_loc(error_str, error);
}

static void _log_fn_arg_pos_error(Error *error) {
  FnArgPosError *f = error->fn_arg_pos;

  str error_str = str_format("positional argument used after named arguments");
  _log_error_with_loc(error_str, error);
}

static void _log_fn_param_req_error(Error *error) {
  FnParamReqError *f = error->fn_param_req;

  str error_str = str_format(
    "required parameter '%s' of function '%s' is not set",
    f->param->name->lexeme, f->call->name->lexeme
  );
  _log_error_with_loc(error_str, error);
}

static void _log_compose_error(Error *error) {
  str error_str = str_format(
    "compose expression is not in a composable function body"
  );
  _log_error_with_loc(error_str, error);
}

static void _log_error(Error *error) {
  switch (error->type) {
    case ERR_SYNTAX: _log_syntax_error(error); break;
    case ERR_IMPORT: _log_import_type_error(error); break;
    case ERR_REDEF: _log_redefinition_error(error); break;
    case ERR_UNDEF: _log_undef_symbol_error(error); break;
    case ERR_TYPE: _log_type_error(error); break;
    case ERR_FN_ARG_COUNT: _log_fn_arg_count_error(error); break;
    case ERR_FN_ARG_NAME: _log_fn_arg_name_error(error); break;
    case ERR_FN_ARG_POS: _log_fn_arg_pos_error(error); break;
    case ERR_FN_PARAM_REQ: _log_fn_param_req_error(error); break;
    case ERR_COMPOSE: _log_compose_error(error); break;
  }
}

bool err_gate() {
  u32 errors = 0;

  for_each(error, e->errors) {
    if (error->level == ERR_ERROR) errors++;
    _log_error(error);
  }

  if (errors > 0) {
    logger_log(e->logger, LOG_FATAL,
      R "Found %u error%s; stopped." RESET,
      errors, errors > 1 ? "s" : ""
    );
  }

  da_clear(e->errors);
  if (errors > 0) cs->status = STATUS_FAILED;
  return errors == 0;
}

void err_free_error(Error *e) {
  switch (e->type) {
    case ERR_SYNTAX: err_free_syntax_error(e->syntax); break;
    case ERR_IMPORT: err_free_import_error(e->import_type); break;
    case ERR_REDEF: err_free_redef_error(e->redefinition); break;
    case ERR_UNDEF: err_free_undef_error(e->undef_symbol); break;
    case ERR_TYPE: err_free_type_error(e->type_error); break;
    case ERR_FN_ARG_COUNT: err_free_fn_arg_count_error(e->fn_arg_count); break;
    case ERR_FN_ARG_NAME: err_free_fn_arg_name_error(e->fn_arg_name); break;
    case ERR_FN_ARG_POS: err_free_fn_arg_pos_error(e->fn_arg_pos); break;
    case ERR_FN_PARAM_REQ: err_free_fn_param_req_error(e->fn_param_req); break;
    case ERR_COMPOSE: break;
  }
}

void err_free_syntax_error(SyntaxError *e) {
  for (u32 i = 0; i < e->expected_len; i++) {
    free(e->expected[i]);
  }
  free(e->expected);
  free(e->found);
  free(e);
}

void err_free_import_error(ImportError *e) {
  free(e->type);
  free(e);
}

void err_free_redef_error(RedefError *e) {
  free(e->object_name);
  free(e);
}

void err_free_undef_error(UndefError *e) {
  free(e->object_name);
  free(e);
}

void err_free_type_error(TypeError *e) {
  free(e->message);
  free(e);
}

void err_free_fn_arg_count_error(FnArgCountError *e) {
  free(e);
}

void err_free_fn_arg_name_error(FnArgNameError *e) {
  free(e);
}

void err_free_fn_arg_pos_error(FnArgPosError *e) {
  free(e);
}

void err_free_fn_param_req_error(FnParamReqError *e) {
  free(e);
}
