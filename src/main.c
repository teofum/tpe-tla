#include <ast/ast.h>
#include <ast/dot.h>
#include <error/error.h>
#include <frontend/frontend.h>
#include <support/fs.h>
#include <support/logger.h>
#include <support/options.h>
#include <support/state.h>
#include <support/types.h>
#include <support/util.h>

static const char *frog =
  "       _     _\n"
  "      (')-=-(')\n"
  "    __(   \"   )__\n"
  "   / _/'-----'\\_ \\\n"
  "___\\\\ \\\\     // //___\n"
  ">____)/_\\---/_\\(____<";

i32 main(i32 argc, char *const*argv) {
  CompilerOptions opts = options(argc, argv);
  if (opts.task == TASK_HELP) {
    printf("%s\n\n", frog);
    printf("Ribbit.\n");
    return 0;
  }

  CompilerState cs = {
    .options = opts,
    .status = STATUS_NOT_STARTED,
    .asts = new_array(Program *, opts.input_file_count),
    .logger = logger_create("Main", stdout, opts.log_level),
    .current_file = -1,
  };
  logger_set_flags(cs.logger, LOGGER_LOG_NAME, false);

  // Initialize compiler
  err_init(&cs);
  fe_init(&cs);

  // Run parser
  for (u32 i = 0; i < cs.options.input_file_count; i++) {
    cs.status = fe_parse(i);
    if (cs.status != STATUS_SUCCEEDED) break;

    // AST graphviz output for debugging
    if (cs.options.emit_ast_dot) ast_generate_dot(&cs, i);
  }

  // First error gate: post-parse
  err_gate();

  // TODO: backend

  // Shutdown compiler
  for (u32 i = 0; i < cs.options.input_file_count; i++) {
    ast_free_program(cs.asts[i]);
  }
  free(cs.asts);
  logger_free(cs.logger);
  options_free(&cs.options);

  fe_shutdown();
  err_shutdown();

  return cs.status;
}
