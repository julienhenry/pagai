BIBTEXSRCS= pagai.bib
 
GNUPLOT=gnuplot

-include LaTeX.mk

pagai.pdf: gnuplot/techniques.tex

gnuplot/%.pdf: gnuplot/%.eps
	epstopdf --filter < $< > $@

gnuplot/%.eps gnuplot/%.tex: gnuplot/%.gnuplot
	cd gnuplot/ && $(GNUPLOT) $*.gnuplot && sed 's#$*#gnuplot\/$*#g' < $*.tex > $*.tex


