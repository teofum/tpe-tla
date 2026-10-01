#! /bin/bash

set -euo pipefail

BASE_PATH="$(dirname "$0")/.."
cd "$BASE_PATH"

.build/frog $@
