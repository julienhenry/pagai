
ROOT=..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

CFILES=${wildcard *.c}
BCFILES=$(CFILES:%.c=bin/%.bc)
RESFILES=$(CFILES:%.c=res/%.res)

all: $(BCFILES)

bin/%.bc: %.c
	$(COMPILE_LLVM) -i $*.c -o $@

res/%.res: bin/%.bc
	$(PAGAI_SH) -i bin/$*.bc -o $@

clean:
	rm bin/*.bc
	rm res/*.res

test: $(RESFILES)
	mkdir res

