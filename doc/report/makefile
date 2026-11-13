latex:
	mkdir -p out
	cd src && lualatex -shell-escape main
	cd src && bibtex main
	cd src && lualatex -shell-escape main
	cd src && lualatex -shell-escape main
	mv src/main.pdf report.pdf
	rm -f src/main.aux src/main.log src/main.out src/main.toc src/main.bbl src/main.blg

watch:
	fswatch -e ".*" -i "\.tex" -o ./ | xargs -n1 -I{} make latex
