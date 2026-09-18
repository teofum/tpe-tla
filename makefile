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
	rm -f src/main/c/frontend/flex/flex_scanner.c
	rm -f src/main/c/frontend/flex/flex_scanner.h
	rm -f src/main/c/frontend/bison/bison_parser.c
	rm -f src/main/c/frontend/bison/bison_parser.h
	echo Cleaned.

clean-build: clean .build/Flex-Bison-Compiler

configure: clean .build

test: .build/Flex-Bison-Compiler
	docker compose run -q --rm compiler src/main/bash/test.sh

run: .build/Flex-Bison-Compiler
	docker compose run -q --rm compiler src/main/bash/run.sh $(src)

.PHONY: clean clean-build configure test
