
ROOT=..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

CFILES=${wildcard *.c}
BCFILES=$(CFILES:%.c=bin/%.bc)
RESFILES=$(CFILES:%.c=res/%.res)

all: $(BCFILES)

bin/%.bc: %.c
	$(COMPILE_LLVM) -g -i $*.c -o $@

res/%.res: bin/%.bc
	$(PAGAI_SH) -i bin/$*.bc -a " -o $@ "

realclean:
	rm bin/*.bc
	rm res/*.res

clean:
	rm res/*.res

test: $(RESFILES)
	if [ ! -d res ] ; then mkdir res; fi

