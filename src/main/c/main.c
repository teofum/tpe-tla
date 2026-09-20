#include <ast/ast.h>
#include <support/types.h>

#include <frontend/frontend.h>

i32 main(i32 argc, const char **argv) {
  CompilerState cs = {
    .ast = NULL,
  };

  fe_init(&cs);

  CompilationStatus parse_status = fe_parse();

  ast_free_program(cs.ast);
  fe_shutdown();

  return parse_status;
}
