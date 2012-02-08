CFILES=${wildcard *.c}
BCFILES=$(CFILES:%.c=bin/%.bc)
RESFILES=$(CFILES:%.c=res/%.res)

all: $(BCFILES)

bin/%.bc: %.c
	./compile_llvm.sh -i $*.c -o $@

res/%.res: bin/%.bc
	../src/pagai -i bin/$*.bc -o $@

clean:
	rm bin/*.bc
	rm res/*.res

test: $(RESFILES)
	mkdir res

