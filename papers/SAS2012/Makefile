implicitization.pdf: gnuplot/techniques.pdf

BIBTEXSRCS= implicitization.bib

GNUPLOT=gnuplot

-include LaTeX.mk

gnuplot/%.pdf: gnuplot/%.eps
	epstopdf --filter < $< > $@

gnuplot/%.eps gnuplot/%.tex: gnuplot/%.gnuplot
	$(GNUPLOT) $<
