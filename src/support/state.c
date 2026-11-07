#include <ast/ast.h>
#include <backend/static_analysis.h>
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
    .symbol_data = new_array(Symbols *, options->input_file_count),
    .values = new_array(const Value *, options->input_file_count),
  };
  logger_set_flags(_cs->logger, LOGGER_LOG_NAME, false);
  for (u32 i = 0; i < _cs->options.input_file_count; i++) {
    _cs->file_dependencies[i] = new_array(bool, options->input_file_count);
  }
  return _cs;
}

CompilationStatus cs_shutdown() {
  for (u32 i = 0; i < _cs->options.input_file_count; i++) {
    ast_free_program(_cs->asts[i]);
    sa_free_symbols(_cs->symbol_data[i]);
    free(_cs->file_dependencies[i]);
  }
  free(_cs->asts);
  free(_cs->symbol_data);
  free(_cs->values);
  free(_cs->file_dependencies);
  free(_cs->file_order);

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

const char *cs_current_filepath() {
  return _cs->options.input_filenames[_cs->current_file];
}
