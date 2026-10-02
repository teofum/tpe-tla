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

typedef struct {
  dyn_array *errors;
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
  e->errors = new_dyn_array_d(Error, err_free_error);
  e->logger = cs->logger;
}

void err_shutdown() {
  dyn_array_free(e->errors);
  free(e);

  e = NULL;
  cs = NULL;
}

void err_report_syntax(SyntaxError *s, Location *location, ErrorLevel level) {
  Error error = {
    .type = ERR_SYNTAX,
    .stage = CS_PARSE,
    .level = level,
    .syntax = s,
    .loc = *location,
    .filepath = state_current_filepath(cs),
  };
  dyn_array_push(e->errors, &error);
}

void err_report_import_type(ImportTypeError *i, Location *location, ErrorLevel level) {
  Error error = {
    .type = ERR_IMPORT_TYPE,
    .stage = CS_PARSE,
    .level = level,
    .import_type = i,
    .loc = *location,
    .filepath = state_current_filepath(cs),
  };
  dyn_array_push(e->errors, &error);
}

void _log(Error *error, const char *const format, ...) {
  va_list args;
  va_start(args, format);
  logger_logv(e->logger, _error_to_log_level[error->level], format, args);
  va_end(args);
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
  str with_loc = str_format(
    FMT_LOCATION "%.*s",
    error->filepath, error->loc.first_line, error->loc.first_column,
    error_str.len, error_str.ptr
  );

  FILE *file = fopen(error->filepath, "r");
  if (file) {
    u32 fl = error->loc.first_line - 1, fc_fl = error->loc.first_column - 1;
    u32 ll = error->loc.last_line - 1, lc_ll = error->loc.last_column - 1;
    char buf[256];
    for (u32 line = 0; line < fl; line++) {
      fgets(buf, 256, file);
    }
    for (u32 line = fl; line <= ll; line++) {
      fgets(buf, 256, file);
      u32 fc = (line == fl) ? fc_fl : 0;
      u32 lc = (line == ll) ? lc_ll : strlen(buf);
      if (lc == 0) continue;

      str_append_temp(&with_loc,
        str_format(INDENTED_NL "%4u| %.*s" R "%.*s" RESET "%s",
          line + 1, fc, buf, lc - fc, buf + fc, buf + lc
        )
      );
      for (u32 i = 0; i < lc; i++) {
        buf[i] = i < fc ? ' ' : i == fc ? '^' : '~';
      }
      buf[lc] = 0;
      str_append_temp(&with_loc, str_format(INDENT R "      %s" RESET, buf));
    }
    fclose(file);
  }

  _log(error, "%.*s", with_loc.len, with_loc.ptr);
  str_free(with_loc);
  str_free(error_str);
}

static void _log_import_type_error(Error *error) {
  ImportTypeError *it = error->import_type;

  str accepted = str_format("\"%s\"", import_type_str[0]);
  for (u32 i = 1; i < IMPORT_TYPE_COUNT; i++) {
    str_append_temp(&accepted, str_format(", \"%s\"", import_type_str[i]));
  }

  _log(error,
    FMT_LOCATION "unrecognized import type \"%s\""
      INDENTED_NL "Accepted import types are: %.*s",
    error->filepath, error->loc.first_line, error->loc.first_column,
    it->type, accepted.len, accepted.ptr
  );
  str_free(accepted);
}

static void _log_error(Error *error) {
  switch (error->type) {
    case ERR_SYNTAX: _log_syntax_error(error); break;
    case ERR_IMPORT_TYPE: _log_import_type_error(error); break;
  }
}

bool err_gate() {
  u32 errors = 0;

  for_each(Error, error, e->errors) {
    if (error->level == ERR_ERROR) errors++;
    _log_error(error);
  }

  if (errors > 0) {
    logger_log(e->logger, LOG_FATAL,
      R "Found %u error%s; stopped." RESET,
      errors, errors > 1 ? "s" : ""
    );
  }

  dyn_array_clear(e->errors);
  if (errors > 0) cs->status = STATUS_FAILED;
  return errors == 0;
}

void err_free_error(Error *e) {
  switch (e->type) {
    case ERR_SYNTAX: err_free_syntax_error(e->syntax); break;
    case ERR_IMPORT_TYPE: err_free_import_type_error(e->import_type); break;
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

void err_free_import_type_error(ImportTypeError *e) {
  free(e->type);
  free(e);
}
