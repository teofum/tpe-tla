#include <support/types.h>

#include <frontend/frontend.h>

i32 main(i32 argc, const char **argv) {
  fe_init();
  CompilationStatus parse_status = fe_parse();
  fe_shutdown();
  return parse_status;
}
