#include <ast/ast.h>
#include <ast/dot.h>
#include <error/error.h>
#include <frontend/frontend.h>
#include <stdio.h>
#include <support/env.h>
#include <support/types.h>

i32 main(i32 argc, const char **argv) {
  // Configuration
  bool emit_ast_dot = env_bool("EMIT_AST_DOT", false);

  CompilerState cs = {
    .ast = NULL,
  };

  // Initialize compiler
  err_init();
  fe_init(&cs);

  // Run parser
  CompilationStatus status = fe_parse();
  fe_parser_log(LOG_INFO, "Parsing done");

  // AST graphviz output for debugging
  if (emit_ast_dot) {
    FILE *dot_output = fopen("ast.gv", "w");
    ast_generate_dot(cs.ast, dot_output);
    fclose(dot_output);
  }

  // First error gate: post-parse
  if (!err_gate()) {
    status = FAILED;
  }

  // Shutdown compiler
  ast_free_program(cs.ast);

  fe_shutdown();
  err_shutdown();

  return status;
}
