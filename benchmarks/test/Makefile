BENCHDIR=.

-include config.mk

ROOT=../..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

CFILES=${wildcard $(BENCHDIR)/*.c}
BCFILES=$(CFILES:%.c=%.bc)
RESFILES=$(CFILES:%.c=%.res)
COMPAREFILES=$(CFILES:%.c=%.res.compare)

all: $(BCFILES)

%.bc: %.c
	$(AT)$(COMPILE_LLVM) -i $*.c -o $@

%.res: %.bc
	$(AT)$(PAGAI_SH) -i $*.bc -o $@ -t 10 2> /dev/null

%.res.compare: %.bc
	$(AT)$(PAGAI_SH) -i $*.bc -c -o $@ -t 10 2> /dev/null

clean:
	rm $(BENCHDIR)/*.bc
	rm $(BENCHDIR)/*.res*

test: $(RESFILES)

compare: $(COMPAREFILES)

