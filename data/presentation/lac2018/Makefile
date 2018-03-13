SAMPLES=lac2018.pdf

all: $(SAMPLES)

clean: cleantmp
	rm -f *.pdf

cleantmp:
	rm -f *.aux
	rm -f *.bbl
	rm -f *.blg
	rm -f *.log

%.pdf: %.tex
	pdflatex "$<"

.PHONY:all clean
