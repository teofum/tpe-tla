[![✗](https://github.com/teofum/tpe-tla/actions/workflows/ci.yaml/badge.svg?branch=development)](https://github.com/teofum/tpe-tla/actions/workflows/ci.yaml)

# Flex-Bison-Compiler

A base compiler example, developed with Flex and Bison.

- [Requirements](#requirements)
- [Configuration](#configuration)
- [Commands](#commands)

## Requirements

- [Docker v29.7.2](https://www.docker.com/)

## Configuration

Set the following environment variables to control and configure the behaviour of the application:

| Name                  | Default | Description                                                                                                                                                           |
| :-------------------- | :-----: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ENVIRONMENT`         | `Local` | The active environment name. The available environments are: `Local`, `Development` and `Production`.                                                                 |
| `LOG_IGNORED_LEXEMES` | `true`  | When `true`, logs all of the ignored lexemes found with Flex at `DEBUGGING` level. To remove those logs from the console output set it to `false`.                    |
| `LOGGING_LEVEL`       |  `ALL`  | The minimum level to log in the console output. From lower to higher, the available levels are: `ALL`, `DEBUGGING`, `INFORMATION`, `WARNING`, `ERROR` and `CRITICAL`. |

_Docker Compose_ can read the variables from an `.env` file too (see `compose.yaml` file).

## Commands

### Start

Rises an ephemeral container, ready to start development:

```bash
docker compose run --rm compiler
```

### Build

Builds or rebuilds the entire compiler:

```bash
.script/build.sh
```

### Run

Compiles a program:

```bash
.script/run.sh <program>
```

where `<program>` is the path to the file that represents its entry-point.

### Test

Executes every available unit-test under `test` folder:

```bash
.script/test.sh
```
