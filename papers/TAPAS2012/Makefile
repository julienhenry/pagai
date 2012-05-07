BIBTEXSRCS= pagai.bib

GNUPLOT=gnuplot

-include LaTeX.mk

gnuplot/%.pdf: gnuplot/%.eps
	epstopdf --filter < $< > $@

gnuplot/%.eps gnuplot/%.tex: gnuplot/%.gnuplot
	cd gnuplot/ && $(GNUPLOT) $*.gnuplot


