#! /bin/bash

set -u
shopt -s globstar nullglob

BASE_PATH="$(dirname "$0")/.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
OFF='\033[0m'
RED='\033[0;31m'
STATUS=0
YELLOW='\033[0;33m'

echo "Compiler will ignore these tests:"
echo ""

for path in test/ignore/**/*; do
	if [ -f "$path" ]; then
		TEST="${path#test/ignore/}"
		echo -e "    ${YELLOW}\"$TEST\"${OFF}"
	fi
done
echo ""

echo "Compiler should accept..."
echo ""

for path in test/accept/**/*; do
	if [ -f "$path" ]; then
		TEST="${path#test/accept/}"
		".build/Flex-Bison-Compiler" < "$path" >/dev/null 2>&1
		RESULT="$?"
		if [ "$RESULT" == "0" ]; then
			echo -e "    \"$TEST\", ${GREEN}and it does${OFF} (status $RESULT)"
		else
			STATUS=1
			echo -e "    \"$TEST\", ${RED}but it rejects${OFF} (status $RESULT)"
		fi
	fi
done
echo ""

echo "Compiler should reject..."
echo ""

for path in test/reject/**/*; do
	if [ -f "$path" ]; then
		TEST="${path#test/reject/}"
		".build/Flex-Bison-Compiler" < "$path" >/dev/null 2>&1
		RESULT="$?"
		if [ "$RESULT" != "0" ]; then
			echo -e "    \"$TEST\", ${GREEN}and it does${OFF} (status $RESULT)"
		else
			STATUS=1
			echo -e "    \"$TEST\", ${RED}but it accepts${OFF} (status $RESULT)"
		fi
	fi
done
echo ""

echo "All done."
exit $STATUS
