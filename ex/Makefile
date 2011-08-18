CFILES=${wildcard *.c}
BCFILES=$(CFILES:%.c=bin/%.bc)

all: $(BCFILES)

bin/%.bc: %.c
	./compile_llvm.sh -i $*.c -o $@
