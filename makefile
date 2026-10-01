GREEN=\033[0;32m
OFF=\033[0m

ifndef VERBOSE
.SILENT:
endif

.build/frog: .build
	docker compose run -q --rm compiler make -C .build
	echo "${GREEN}Build successful.${OFF}"

.build:
	docker compose run -q --rm compiler cmake -S . -B .build
	echo "${GREEN}Configure successful.${OFF}"

clean:
	rm -rf .build
	rm -f src/frontend/flex/flex_scanner.c
	rm -f src/frontend/flex/flex_scanner.h
	rm -f src/frontend/bison/bison_parser.c
	rm -f src/frontend/bison/bison_parser.h
	echo Cleaned.

clean-build: clean .build/frog

rebuild:
	rm -f .build/frog
	make

configure: clean .build

test: .build/frog
	docker compose run -q --rm compiler .script/test.sh

run: .build/frog
	docker compose run -q --rm compiler .script/run.sh $(src)

.PHONY: clean clean-build configure test
