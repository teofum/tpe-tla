#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <support/env.h>
#include <support/hash_map.h>
#include <support/logger.h>
#include <support/types.h>
#include <support/util.h>

#include "options.h"

static CompilerOptions _options_default() {
  CompilerOptions opts = {
    .task = TASK_COMPILE,
    .log_level = LOG_INFO,

    .input_file_count = 0,
    .input_filenames = NULL,
    .base_path = getcwd(NULL, 0),
    .base_url = strdup(""),

    .emit_ast_dot = false,
    .ast_dot_base_path = NULL,

    .parser_log_level = LOG_NONE,
    .scanner_log_level = LOG_NONE,
    .symbols_log_level = LOG_NONE,
    .typecheck_log_level = LOG_NONE,

    .definitions = new(CompilerDefinitions),
  };

  map_init(opts.definitions, hash_cstring, compare_cstring);
  map_manage_keys(opts.definitions, NULL, (void(*)(char *))free);
  map_manage_values(opts.definitions, (void(*)(char *))free);

  return opts;
};

static void _options_env(CompilerOptions *options) {
  env_set_log_level("LOG_LEVEL", &options->log_level);
  env_set_bool("EMIT_AST_DOT", &options->emit_ast_dot);
  env_set_str("AST_DOT_BASE_PATH", &options->ast_dot_base_path);

  env_set_log_level("__DEBUG_SCANNER_LOG_LEVEL", &options->scanner_log_level);
  env_set_log_level("__DEBUG_PARSER_LOG_LEVEL", &options->parser_log_level);
  env_set_log_level("__DEBUG_SYMBOLS_LOG_LEVEL", &options->symbols_log_level);
  env_set_log_level("__DEBUG_TYPECHECK_LOG_LEVEL", &options->typecheck_log_level);
}

static void _options_cli_args(CompilerOptions *options, i32 argc, char *const*argv) {
  enum {
    OPT_LOG_LEVEL = 256,
    OPT_AST_DOT,
  };

  static const char *optstr = "v::D:b:u:qh";
  static struct option long_opts[] = {
    {"help",        no_argument,        NULL, 'h'           },
    {"verbose",     optional_argument,  NULL, 'v'           },
    {"quiet",       no_argument,        NULL, 'q'           },
    {"log",         required_argument,  NULL, OPT_LOG_LEVEL },
    {"emit-ast",    optional_argument,  NULL, OPT_AST_DOT   },
    {"define",      required_argument,  NULL, 'D'           },
    {"base-dir",    required_argument,  NULL, 'b'           },
    {"base-url",    required_argument,  NULL, 'u'           },
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
      case 'D': {
        char *eq = strchr(optarg, '=');
        if (eq) {
          char *key = strndup(optarg, eq - optarg);
          char *value = strdup(eq + 1);
          map_insert(options->definitions, key, value);
        } else {
          fprintf(stderr, "Invalid definition string '%s'\n", optarg);
        }
        break;
      }
      case 'b': {
        if (options->base_path) free(options->base_path);
        options->base_path = realpath(optarg, NULL);
      }
      case 'u': {
        if (options->base_url) free(options->base_url);
        options->base_url = strdup(optarg);
      }
      case OPT_LOG_LEVEL:
        options->log_level = logger_str_to_level(optarg);
        break;
      case OPT_AST_DOT:
        options->emit_ast_dot = true;
        if (optarg) options->ast_dot_base_path = strdup(optarg);
        break;
    }
  }

  if (options->task != TASK_COMPILE) return;

  if (optind >= argc) {
    fprintf(stderr, "Expected input file after options\n");
  } else {
    options->input_file_count = argc - optind;
    options->input_filenames = new_array(char *, options->input_file_count);
    for (u32 i = 0; i < options->input_file_count; i++)
    options->input_filenames[i] = argv[i + optind];
  }
}

CompilerOptions options(i32 argc, char *const*argv) {
  CompilerOptions options = _options_default();
  _options_env(&options);
  _options_cli_args(&options, argc, argv);

  return options;
}

void options_free(CompilerOptions *opts) {
  map_free(opts->definitions);
  free(opts->definitions);
  free(opts->input_filenames);
  free(opts->base_path);
  free(opts->base_url);
  if (opts->ast_dot_base_path) free(opts->ast_dot_base_path);
}
