#!/opt/homebrew/bin/fish

set -l file $argv[1]
make run src=test/$file
dot -Tpng ast.gv -o ast.png
open ast.png
