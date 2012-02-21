BENCHDIR=.

-include config.mk

BCFILES=${wildcard $(BENCHDIR)/*.bc}
RESFILES=$(BCFILES:%.bc=%.res)
COMPAREFILES=$(BCFILES:%.bc=%.res.compare)

all: $(RESFILES)

%.res: %.bc
	@pagai.sh -i $*.bc -o $@ -t 10 2> /dev/null

%.res.compare: %.bc
	@pagai.sh -i $*.bc -c -o $@ -t 10 2> /dev/null

clean:
	rm $(BENCHDIR)/*.res*

test: $(RESFILES)
	
compare: $(COMPAREFILES)
	

