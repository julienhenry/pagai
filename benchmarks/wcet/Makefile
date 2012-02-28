BENCHDIR=.

-include config.mk

BCFILES=${wildcard $(BENCHDIR)/*.bc}
RESFILES=$(BCFILES:%.bc=%.res)
COMPAREFILES=$(BCFILES:%.bc=%.res.compare)
NCOMPAREFILES=$(BCFILES:%.bc=%.res.narrow)

all: $(RESFILES)

%.res: %.bc
	@pagai.sh -s -I 2000 -t 800 -i $*.bc > $@

%.res.compare: %.bc
	@pagai.sh -s -t 800 -i $*.bc -a " -c " > $@

%.res.narrow: %.bc
	@pagai.sh -s -I 20000 -t 800 -i $*.bc -a " -t s --domain pk --domain2 pk -M " > $@

clean:
	rm $(BENCHDIR)/*.res*

cclean:
	rm $(BENCHDIR)/*.res.compare

nclean:
	rm $(BENCHDIR)/*.res.narrow

test: $(RESFILES)
	
compare: $(COMPAREFILES)

ncompare: $(NCOMPAREFILES)

ndiff:
	@./diff_narrowing.sh $(BENCHDIR)
	
cdiff:
	@./diff_compare.sh $(BENCHDIR)

