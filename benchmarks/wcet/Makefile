BENCHDIR=.

-include config.mk

BCFILES=${wildcard $(BENCHDIR)/*.bc}
RESFILES=$(BCFILES:%.bc=%.res)
COMPAREFILES=$(BCFILES:%.bc=%.res.compare)

all: $(RESFILES)

%.res: %.bc
	@pagai.sh -i $*.bc -o $@ -t 800 2> /dev/null

%.res.compare: %.bc
	@echo $*.bc started...
	@pagai.sh -i $*.bc -c -t 800 > $@ 2> /dev/null
	@echo $*.bc finished

clean:
	rm $(BENCHDIR)/*.res*

test: $(RESFILES)
	
compare: $(COMPAREFILES)
	

