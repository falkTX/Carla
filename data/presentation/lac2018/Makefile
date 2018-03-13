SAMPLES=lac2018.pdf

all: $(SAMPLES)

clean: cleantmp
	rm -f *.pdf

cleantmp:
	rm -f *.aux
	rm -f *.bbl
	rm -f *.blg
	rm -f *.log

%.pdf: %.tex # %.bib
	pdflatex "$<"
	bibtex `basename "$<" .tex`
	pdflatex "$<"
	pdflatex "$<"

.PHONY:all clean
