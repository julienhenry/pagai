BENCHDIR=.

-include config.mk

BCFILES=${wildcard $(BENCHDIR)/*.bc}
RESFILES=$(BCFILES:%.bc=%.res)
COMPAREFILES=$(BCFILES:%.bc=%.res.compare)
NCOMPAREFILES=$(BCFILES:%.bc=%.res.narrow)

all: $(RESFILES)

%.res: %.bc
	@pagai.sh -i $*.bc -o $@ -t 800 2> /dev/null

%.res.compare: %.bc
	@echo $*.bc started...
	@pagai.sh -i $*.bc -c -t 800 > $@ 2> /dev/null
	@echo $*.bc finished

%.res.narrow: %.bc
	@echo $*.bc started...
	@pagai.sh -s -I 2000 -t 800 -i $*.bc -a " -t s --domain pk --domain2 pk -M " > $@
	@echo $*.bc finished
clean:
	rm $(BENCHDIR)/*.res*

cclean:
	rm $(BENCHDIR)/*.res,compare

nclean:
	rm $(BENCHDIR)/*.res.narrow

test: $(RESFILES)
	
compare: $(COMPAREFILES)

ncompare: $(NCOMPAREFILES)

ndiff:
	@./diff_narrowing.sh $(BENCHDIR)
	

