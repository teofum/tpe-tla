#! /bin/bash

set -euo pipefail

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
OFF='\033[0m'

bison \
	-Wcounterexamples \
	-d "src/main/c/frontend/bison/bison_grammar.y" \
	--output="src/main/c/frontend/bison/bison_parser.c"

echo -e "${GREEN}Bison done.${OFF}"
