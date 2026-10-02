#include "state.h"

const char *state_current_filepath(CompilerState *cs) {
  return cs->options.input_filenames[cs->current_file];
}
