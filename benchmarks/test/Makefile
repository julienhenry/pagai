BENCHDIR=.

-include config.mk


CFILES=${wildcard $(BENCHDIR)/*.c}
BCFILES=$(CFILES:%.c=%.bc)
RESFILES=$(CFILES:%.c=%.res)
COMPAREFILES=$(CFILES:%.c=%.res.compare)

all: $(BCFILES)

%.bc: %.c
	@./compile_llvm.sh -i $*.c -o $@

%.res: %.bc
	@pagai.sh -i $*.bc -o $@ -t 10 2> /dev/null

%.res.compare: %.bc
	@pagai.sh -i $*.bc -c -o $@ -t 10 2> /dev/null

clean:
	rm $(BENCHDIR)/*.bc
	rm $(BENCHDIR)/*.res*

test: $(RESFILES)
	

compare: $(COMPAREFILES)
	

