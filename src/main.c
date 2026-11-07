#include "backend/builtins.h"
#include <ast/ast.h>
#include <ast/symbols.h>
#include <ast/dot.h>
#include <backend/codegen.h>
#include <backend/eval.h>
#include <backend/static_analysis.h>
#include <error/error.h>
#include <frontend/frontend.h>
#include <stdlib.h>
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

CompilerState *init(CompilerOptions *opts);
CompilationStatus shutdown();

i32 main(i32 argc, char *const*argv) {
  CompilerOptions opts = options(argc, argv);
  if (opts.task == TASK_HELP) {
    printf("%s\n\n", frog);
    printf("Ribbit.\n");
    return 0;
  }

  CompilerState *cs = init(&opts);

  // Run parser
  for (u32 i = 0; i < cs->options.input_file_count; i++) {
    cs->status = fe_parse(i);
    if (cs->status != STATUS_SUCCEEDED) break;

    // AST graphviz output for debugging
    if (cs->options.emit_ast_dot) ast_generate_dot(cs, i);
  }
  if (!err_gate()) return shutdown();

  for (u32 i = 0; i < cs->options.input_file_count; i++) {
    logger_log(cs->logger, LOG_DEBUG, "%s depends on:", cs->options.input_filenames[i]);
    for (u32 j = 0; j < cs->options.input_file_count; j++) {
      if (cs->file_dependencies[i][j]) {
        logger_log(cs->logger, LOG_DEBUG, "\t%s", cs->options.input_filenames[j]);
      }
    }
  }

  cs_build_file_order();
  if (cs->status != STATUS_SUCCEEDED) return shutdown();

  logger_log(cs->logger, LOG_DEBUG, "File order:");
  for (u32 i = 0; i < cs->options.input_file_count; i++) {
    logger_log(cs->logger, LOG_DEBUG, "\t%s", cs->options.input_filenames[cs->file_order[i]]);
  }

  // Build static (type/function) symbol tables
  for (u32 i = 0; i < cs->options.input_file_count; i++) {
    sa_build_static_tables(cs->file_order[i]);
  }
  if (!err_gate()) return shutdown();

  // Typecheck and check reference integrity
  for (u32 i = 0; i < cs->options.input_file_count; i++) {
    sa_build_tables_and_typecheck(cs->file_order[i]);
  }
  if (!err_gate()) return shutdown();

  // Evaluate
  for (u32 i = 0; i < cs->options.input_file_count; i++) {
    eval_evaluate(cs->file_order[i]);
  }
  if (!err_gate()) return shutdown();

  // Evaluate
  for (u32 i = 0; i < cs->options.input_file_count; i++) {
    cg_generate(cs->file_order[i]);
  }
  if (!err_gate()) return shutdown();

  return shutdown();
}

CompilerState *init(CompilerOptions *opts) {
  CompilerState *cs = cs_init(opts);

  err_init();
  fe_init();
  builtins_init();
  sa_init();
  eval_init();
  cg_init();

  return cs;
}

CompilationStatus shutdown() {
  cg_shutdown();
  eval_shutdown();
  sa_shutdown();
  builtins_shutdown();
  fe_shutdown();
  err_shutdown();

  return cs_shutdown();
}
