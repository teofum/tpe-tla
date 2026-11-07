#include <stdlib.h>

#include <ast/ast.h>
#include <backend/static_analysis.h>
#include <backend/scope.h>
#include <support/logger.h>
#include <support/types.h>
#include <support/util.h>

#include "state.h"

CompilerState *_cs;

CompilerState *cs_init(CompilerOptions *options) {
  _cs = new(CompilerState);
  *_cs = (CompilerState){
    .options = *options,
    .logger = logger_create("Main", stdout, options->log_level),
    .file_dependencies = new_array(bool *, options->input_file_count),
    .file_order = new_array(u32, options->input_file_count),
    .status = STATUS_NOT_STARTED,
    .current_file = -1,
    .asts = new_array(Program *, options->input_file_count),
    .values = new_array(const Value *, options->input_file_count),
    .file_scopes = new_array(Scope, options->input_file_count),
    .symbols = new(SymbolTable),
  };
  logger_set_flags(_cs->logger, LOGGER_LOG_NAME, false);
  for (u32 i = 0; i < _cs->options.input_file_count; i++) {
    _cs->file_dependencies[i] = new_array(bool, options->input_file_count);
  }

  map_init(_cs->symbols, sk_hash, sk_compare);
  map_manage_keys(_cs->symbols, sk_clone, sk_free);
  map_manage_values(_cs->symbols, (void(*)(Symbol *))free);
  return _cs;
}

CompilationStatus cs_shutdown() {
  for (u32 i = 0; i < _cs->options.input_file_count; i++) {
    ast_free_program(_cs->asts[i]);
    free(_cs->file_dependencies[i]);
  }
  free(_cs->asts);
  sa_free_symbols(_cs->symbols);
  free(_cs->values);
  free(_cs->file_dependencies);
  free(_cs->file_order);
  free(_cs->file_scopes);

  logger_free(_cs->logger);
  options_free(&_cs->options);

  CompilationStatus status = _cs->status;
  free(_cs);
  _cs = NULL;
  return status;
}

CompilerState *cs() {
  return _cs;
}

void cs_build_file_order() {
  u32 fc = _cs->options.input_file_count;
  u32 order = 0;
  bool *resolved = new_array(bool, fc);
  while (order < fc) {
    bool valid = false;
    for (u32 i = 0; i < fc; i++) {
      if (resolved[i]) continue;
      bool file_resolved = true;
      for (u32 j = 0; j < fc; j++) {
        if (i != j && _cs->file_dependencies[i][j] && !resolved[j]) file_resolved = false;
        if (i == j && _cs->file_dependencies[i][j]) {
          // File depends on itself????? ????
          logger_log(
            _cs->logger, LOG_FATAL,
            "%s depends on.. itself????  wat ???? how did you even think this was going to work?????????",
            _cs->options.input_filenames[i]
          );
          _cs->status = STATUS_FAILED;
          free(resolved);
          return;
        }
      }
      if (!file_resolved) continue;

      _cs->file_order[order++] = i;
      resolved[i] = true;
      valid = true;
    }
    if (!valid) {
      logger_log(_cs->logger, LOG_FATAL, "Unable to resolve imports; possible circular dependency.");
      _cs->status = STATUS_FAILED;
      free(resolved);
      return;
    }
  }
  free(resolved);
}

const char *cs_current_filepath() {
  return _cs->options.input_filenames[_cs->current_file];
}
