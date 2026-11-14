#include <getopt.h>
#include <ftw.h>
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
#include "support/dyn_array.h"
#include "support/fs.h"

static CompilerOptions _options_default() {
  CompilerOptions opts = {
    .task = TASK_COMPILE,
    .log_level = LOG_INFO,

    .input_file_count = 0,
    .input_filenames = NULL,
    .base_path = getcwd(NULL, 0),
    .base_url = strdup(""),
    .out_path = strdup("out"),

    .emit_ast_dot = false,
    .ast_dot_base_path = NULL,

    .parser_log_level = LOG_NONE,
    .scanner_log_level = LOG_NONE,
    .symbols_log_level = LOG_NONE,
    .typecheck_log_level = LOG_NONE,
    .eval_log_level = LOG_NONE,

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
  env_set_log_level("__DEBUG_EVAL_LOG_LEVEL", &options->eval_log_level);
}

typedef dyn_array(char *) PathList;
static PathList *_paths = NULL;

static i32 _options_check_file(
  const char *path,
  const struct stat *sb,
  i32 typeflag
) {
  if (typeflag == FTW_F && strcmp(fs_ext(path), "frog") == 0) {
    char *path2 = strdup(path);
    da_push(_paths, &path2);
  }
  return 0;
}

static void _options_cli_args(CompilerOptions *options, i32 argc, char *const*argv) {
  enum {
    OPT_LOG_LEVEL = 256,
    OPT_AST_DOT,
  };

  static const char *optstr = "v::D:s:o:b:u:qh";
  static struct option long_opts[] = {
    {"help",        no_argument,        NULL, 'h'           },
    {"verbose",     optional_argument,  NULL, 'v'           },
    {"quiet",       no_argument,        NULL, 'q'           },
    {"log",         required_argument,  NULL, OPT_LOG_LEVEL },
    {"emit-ast",    optional_argument,  NULL, OPT_AST_DOT   },
    {"define",      required_argument,  NULL, 'D'           },
    {"src-dir",     required_argument,  NULL, 's'           },
    {"out-dir",     required_argument,  NULL, 'o'           },
    {"base-dir",    required_argument,  NULL, 'b'           },
    {"base-url",    required_argument,  NULL, 'u'           },
    {0,             0,                  0,    0             }
  };

  bool defined_basedir = false;
  bool defined_outdir = false;
  bool valid = true;
  char *glob_dir = NULL;

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
        if (defined_basedir) {
          fprintf(stderr, "Multiple base directories defined\n");
          valid = false;
        } else {
          if (options->base_path) free(options->base_path);
          options->base_path = realpath(optarg, NULL);
          defined_basedir = true;
        }
        break;
      }
      case 's': {
        if (glob_dir) {
          fprintf(stderr, "Multiple source directories defined\n");
          valid = false;
        } else {
          glob_dir = realpath(optarg, NULL);
          if (!defined_basedir) {
            if (options->base_path) free(options->base_path);
            options->base_path = realpath(optarg, NULL);
          }
        }
        break;
      }
      case 'o': {
        if (defined_outdir) {
          fprintf(stderr, "Multiple output directories defined\n");
          valid = false;
        } else {
          if (options->out_path) free(options->out_path);
          options->out_path = strdup(optarg);
          defined_outdir = true;
        }
        break;
      }
      case 'u': {
        if (options->base_url) free(options->base_url);
        options->base_url = strdup(optarg);
        break;
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

  if (!valid) return;
  if (options->task != TASK_COMPILE) return;

  if (glob_dir) {
    if (optind < argc) {
      fprintf(stderr, "Unexpected input files with source directory option\n");
    } else {
      _paths = new(PathList);
      da_init(_paths);
      ftw(glob_dir, _options_check_file, 8);

      options->input_file_count = da_size(_paths);
      options->input_filenames = new_array(char *, options->input_file_count);
      u32 i = 0;
      for_each(path, _paths) options->input_filenames[i++] = *path;

      da_free(_paths);
      free(_paths);
    }
    free(glob_dir);
  } else if (optind >= argc) {
    fprintf(stderr, "Expected input files after options\n");
  } else {
    options->input_file_count = argc - optind;
    options->input_filenames = new_array(char *, options->input_file_count);
    for (u32 i = 0; i < options->input_file_count; i++) {
      options->input_filenames[i] = strdup(argv[i + optind]);
    }
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
  for (u32 i = 0; i < opts->input_file_count; i++) {
    free(opts->input_filenames[i]);
  }
  free(opts->input_filenames);
  free(opts->out_path);
  free(opts->base_path);
  free(opts->base_url);
  if (opts->ast_dot_base_path) free(opts->ast_dot_base_path);
}
