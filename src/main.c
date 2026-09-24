#include <ast/ast.h>
#include <ast/dot.h>
#include <frontend/frontend.h>
#include <stdio.h>
#include <support/types.h>

i32 main(i32 argc, const char **argv) {
  CompilerState cs = {
    .ast = NULL,
  };

  fe_init(&cs);

  CompilationStatus parse_status = fe_parse();
  fe_parser_log(LOG_INFO, "Parsing done");

  if (parse_status == SUCCEEDED) {
    FILE *dot_output = fopen("ast.gv", "w");
    ast_generate_dot(cs.ast, dot_output);
    fclose(dot_output);
    ast_free_program(cs.ast);
  }

  fe_shutdown();

  return parse_status;
}
