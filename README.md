[![✗](https://github.com/teofum/tpe-tla/actions/workflows/ci.yaml/badge.svg?branch=development)](https://github.com/teofum/tpe-tla/actions/workflows/ci.yaml)

# frog 🐸

A programming language dedicated to static website generation.

- [Requirements](#requirements)
- [Usage](#usage)
- [Configuration](#configuration)
- [Commands](#commands)

## Requirements

- [Docker v29.7.2](https://www.docker.com/)

## Usage

```bash
frog <options> <file>
```

where `<options>` is a list of command line options described in [Configuration](#configuration) and `<file>` is the path to an input file.

## Configuration

The compiler takes options from environment variables or command line arguments, with the latter having priority if both are defined.

### Command line options

| Command line              | Default       | Description                                                                                                |
| :------------------------ | :------------ | :--------------------------------------------------------------------------------------------------------- |
| `-h`, `--help`            | unset         | Print a help message and exit.                                                                             |
| `-v`, `--verbose`         | unset         | Equivalent to `--log=verbose`.                                                                             |
| `-q`, `--quiet`           | unset         | Equivalent to `--log=none`.                                                                                |
| `--log=<level>`           | `info`        | Set the compiler logging level. See [Logging](#logging) for valid options.                                 |
| `--emit-ast[=<filename>]` | off, `ast.gv` | If set, the compiler will emit a GraphViz DOT file for the full AST after parsing with the given filename. |

### Environment variables

| Name               | Default  | Description                                                                           |
| :----------------- | :------- | :------------------------------------------------------------------------------------ |
| `LOG_LEVEL`        | `info`   | Set the compiler logging level. See [Logging](#logging) for valid options.            |
| `EMIT_AST_DOT`     | `false`  | If `true`, the compiler will emit a GraphViz DOT file for the full AST after parsing. |
| `AST_DOT_FILENAME` | `ast.gv` | Filename for the GraphViz AST debug output.                                           |

_Docker Compose_ can read the variables from an `.env` file too (see `compose.yaml` file).

### Logging

The compiler logging level can be set to one of `fatal`, `error`, `warning`, `info`, `verbose`, `debug`, `none` or `all` (equivalent to `debug`).

## Commands

### Build

To perform an initial build, simply use `make`.

To incrementally rebuild the changed parts:

```bash
make rebuild
```

To do a clean build from scratch (necessary when making changes to Flex or Bison files):

```bash
make clean-build
```

### Run

Compiles a program using an ephemeral container:

```bash
docker compose run -q --rm compiler .script/run.sh <options>
```

where `<options>` are the command line arguments as described in [Usage](#usage).

### Test

Executes every available unit-test under `test` folder:

```bash
make test
```

## FAQ

### why is it named frog

frogs are cool
