LATEX = pdflatex

all: synthesis2012.pdf

SOURCES = $(wildcard *.tex) $(wildcard FIGS/*) synthesis2012.bib Makefile

OBJECTS = synthesis2012.aux synthesis2012.bbl synthesis2012.blg synthesis2012.log synthesis2012.pdf

clean:
	rm -f $(OBJECTS)

synthesis2012.pdf: synthesis2012.bbl $(SOURCES)
	$(LATEX) synthesis2012
	$(LATEX) synthesis2012


synthesis2012.bbl: $(SOURCES)
	$(LATEX) synthesis2012
	bibtex synthesis2012


