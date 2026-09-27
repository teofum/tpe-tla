#!/opt/homebrew/bin/fish

docker compose run -q --rm compiler .script/run.sh $argv
dot -Tpng ast.gv -o ast.png
open ast.png
