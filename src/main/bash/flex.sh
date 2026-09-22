#! /bin/bash

set -euo pipefail

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
OFF='\033[0m'

flex \
	--bison-bridge \
	--bison-locations \
	--header-file="src/main/c/frontend/flex/flex_scanner.h" \
	--never-interactive \
	--nounistd \
	--noyywrap \
	--outfile="src/main/c/frontend/flex/flex_scanner.c" \
	--reentrant \
	--stack \
	--yylineno \
	"src/main/c/frontend/flex/flex_patterns.l"

echo -e "${GREEN}Flex done.${OFF}"
