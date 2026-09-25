#include <stdlib.h>

#include <support/dyn_array.h>

#include "error.h"
#include "support/util.h"

typedef struct {
  dyn_array *errors;
} ErrorState;

static ErrorState *e;

void err_init() {
  e = new(ErrorState);
  e->errors = new_dyn_array_d(Error *, err_free_syntax_error);
}

void err_shutdown() {
  dyn_array_free(e->errors);
  free(e);
  e = NULL;
}

void err_report_syntax(SyntaxError *s) {
  Error *error = new(Error);
  *error = (Error){ .type = ERR_SYNTAX, .syntax = s };
  dyn_array_push(e->errors, &error);
}

bool err_gate() {
  return dyn_array_size(e->errors) == 0;
}

void err_free_error(Error *e) {
  switch (e->type) {
    case ERR_SYNTAX: err_free_syntax_error(e->syntax); break;
  }
  free(e);
}

void err_free_syntax_error(SyntaxError *e) {
  for (u32 i = 0; i < e->expected_len; i++) {
    free(e->expected[i]);
  }
  free(e->expected);
  free(e->found);
}
