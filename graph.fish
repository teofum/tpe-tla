#!/opt/homebrew/bin/fish

set -l gv_path "ast/$argv[-1]"

docker compose run -q --rm compiler .script/run.sh $argv
dot -Tpng $gv_path -o ast.png
open ast.png
