#include <stdio.h>

#include <ast/ast.h>
#include <ast/dot.h>
#include <error/error.h>
#include <frontend/frontend.h>
#include <support/logger.h>
#include <support/options.h>
#include <support/state.h>
#include <support/types.h>

i32 main(i32 argc, char *const*argv) {
  CompilerOptions opts = options(argc, argv);
  if (opts.task == TASK_HELP) {
    printf("TODO: helpful text :)\n");
    return 0;
  }

  CompilerState cs = {
    .options = opts,
    .status = STATUS_NOT_STARTED,
    .ast = NULL,
    .logger = logger_create("Main", stdout, opts.log_level),
    .current_filepath = NULL,
  };
  logger_set_flags(cs.logger, LOGGER_LOG_NAME, false);

  // Initialize compiler
  err_init(&cs);
  fe_init(&cs);

  // Run parser
  cs.status = fe_parse(cs.options.input_filename);

  // AST graphviz output for debugging
  if (cs.options.emit_ast_dot && cs.status == STATUS_SUCCEEDED) {
    FILE *dot_output = fopen(cs.options.ast_dot_filename, "w");
    ast_generate_dot(cs.ast, dot_output);
    fclose(dot_output);
  }

  // First error gate: post-parse
  err_gate();

  // TODO: backend

  // Shutdown compiler
  ast_free_program(cs.ast);
  logger_free(cs.logger);

  fe_shutdown();
  err_shutdown();

  return cs.status;
}
