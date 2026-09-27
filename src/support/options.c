#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include <support/env.h>
#include <support/logger.h>
#include <support/types.h>
#include <support/util.h>

#include "options.h"

static CompilerOptions _options_default() {
  return (CompilerOptions){
    .task = TASK_COMPILE,
    .log_level = LOG_INFO,

    .emit_ast_dot = false,
    .ast_dot_filename = "ast.gv",

    .parser_log_level = LOG_NONE,
    .scanner_log_level = LOG_NONE,
  };
};

static void _options_env(CompilerOptions *options) {
  env_set_log_level("LOG_LEVEL", &options->log_level);
  env_set_bool("EMIT_AST_DOT", &options->emit_ast_dot);
  env_set_str("AST_DOT_FILENAME", &options->ast_dot_filename);

  env_set_log_level("__DEBUG_SCANNER_LOG_LEVEL", &options->scanner_log_level);
  env_set_log_level("__DEBUG_PARSER_LOG_LEVEL", &options->parser_log_level);
}

static void _options_cli_args(CompilerOptions *options, i32 argc, char *const*argv) {
  enum {
    OPT_LOG_LEVEL = 256,
    OPT_AST_DOT,
  };

  static const char *optstr = "v::qh";
  static struct option long_opts[] = {
    {"help",        no_argument,        NULL, 'h'           },
    {"verbose",     optional_argument,  NULL, 'v'           },
    {"quiet",       no_argument,        NULL, 'q'           },
    {"log",         required_argument,  NULL, OPT_LOG_LEVEL },
    {"emit-ast",    optional_argument,  NULL, OPT_AST_DOT   },
    {0,             0,                  0,    0             }
  };

  i32 c;
  while ((c = getopt_long(argc, argv, optstr, long_opts, NULL)) != -1) {
    switch (c) {
      case 'h':
        options->task = TASK_HELP;
        break;
      case 'v':
        options->log_level = logger_str_to_level(or_default(optarg, "verbose"));
        break;
      case 'q':
        options->log_level = logger_str_to_level("none");
        break;
      case OPT_LOG_LEVEL:
        options->log_level = logger_str_to_level(optarg);
        break;
      case OPT_AST_DOT:
        options->emit_ast_dot = true;
        if (optarg) options->ast_dot_filename = strdup(optarg);
        break;
    }
  }
}

CompilerOptions options(i32 argc, char *const*argv) {
  CompilerOptions options = _options_default();
  _options_env(&options);
  _options_cli_args(&options, argc, argv);

  return options;
}
