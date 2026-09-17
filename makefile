GREEN=\033[0;32m
OFF=\033[0m

ifndef VERBOSE
.SILENT:
endif

.build/Flex-Bison-Compiler: .build
	docker compose run -q --rm compiler make -C .build
	echo "${GREEN}Build successful.${OFF}"

.build:
	docker compose run -q --rm compiler cmake -S . -B .build
	echo "${GREEN}Configure successful.${OFF}"

clean:
	rm -rf .build
	rm -f src/main/c/frontend/lexical-analysis/flex_scanner.c
	rm -f src/main/c/frontend/lexical-analysis/flex_scanner.h
	rm -f src/main/c/frontend/syntactic-analysis/bison_parser.c
	rm -f src/main/c/frontend/syntactic-analysis/bison_parser.h
	echo Cleaned.

clean-build: clean .build/Flex-Bison-Compiler

test: .build/Flex-Bison-Compiler
	docker compose run -q --rm compiler src/main/bash/test.sh

.PHONY: clean clean-build test
