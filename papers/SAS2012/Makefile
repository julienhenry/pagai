implicitization.pdf: gnuplot/techniques.pdf table1.tex table_time.tex table_domain.tex

BIBTEXSRCS= implicitization.bib

GNUPLOT=gnuplot

-include LaTeX.mk

gnuplot/%.pdf: gnuplot/%.eps
	epstopdf --filter < $< > $@

gnuplot/%.eps gnuplot/%.tex: gnuplot/%.gnuplot
	cd gnuplot/ && $(GNUPLOT) $*.gnuplot
