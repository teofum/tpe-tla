#include <assert.h>
#include <libgen.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <ast/ast.h>
#include <support/fs.h>
#include <support/state.h>
#include <support/util.h>

#include "codegen.h"
#include "support/hash_map.h"

typedef struct {
  FILE *out_file;
} Codegen;

CompilerState *cs = NULL;
Codegen *cg = NULL;

/* -- Module init/shutdown -------------------------------------------------- */

void cg_init(CompilerState *compiler_state) {
  cs = compiler_state;

  cg = new(Codegen);
}

void cg_shutdown() {
  cs = NULL;
}

/* -- Codegen: utils -------------------------------------------------------- */

static void _p(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(cg->out_file, fmt, args);
  va_end(args);
}

/* -- Codegen: generation --------------------------------------------------- */

static void _write_value(const Value *v, const char *sep) {
  switch (v->type) {
    case VAL_INT: _p("%ld%s", v->integer, sep); break;
    case VAL_FLOAT: _p("%f%s", v->floating, sep); break;
    case VAL_BOOL: _p("%s%s", v->boolean ? "true" : "false", sep); break;
    case VAL_STRING: _p("%.*s%s", v->string->len, v->string->ptr, sep); break;
    case VAL_NIL: break;
    case VAL_ELEMENT: {
      Element *el = v->element;
      _p("<%s", el->tag);

      const char *attr_key;
      const Value *attr_value;
      map_foreach(attr_key, attr_value, el->attributes) {
        _p(" %s=\"", attr_key);
        _write_value(attr_value, "");
        _p("\"");
      }

      _p("%s", el->self_closing ? "/>" : ">");

      if (!el->self_closing) {
        if (el->children) {
          for (u32 i = 0; i < el->children->len; i++) {
            _write_value(
              el->children->items[i],
              i == el->children->len - 1 ? "" : " "
            );
          }
        }
        _p("</%s>", el->tag);
      }
      break;
    }
    case VAL_LIST: {
      for (u32 i = 0; i < v->list->len; i++) {
        _write_value(
          v->list->items[i],
          i == v->list->len - 1 ? "" : " "
        );
      }
      break;
    }
    case VAL_MAP: assert(false); // TODO error
    case VAL_RANGE: assert(false); // TODO error
  }
}

void cg_generate(u32 file_idx) {
  cs->current_file = file_idx;

  const Value *value = cs->values[file_idx];
  if (value != NULL) {
    // char *stem = fs_stem(state_current_filepath(cs));
    // char *out_path = NULL;
    // asprintf(&out_path, "out/%s.html", stem);
    // free(stem);
    // cg->out_file = fopen(out_path, "w");

    cg->out_file = stdout;
    _write_value(value, "");
    fprintf(cg->out_file, "\n");

    // fclose(cg->out_file);
    // free(out_path);
  }

  cs->current_file = -1;
}
