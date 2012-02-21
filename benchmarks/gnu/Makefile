# List of subdirectories:
BENCHDIR=.

-include config.mk

SUBDIRS=$(shell echo $(BENCHDIR)/*/)
TIMESTAMPS=$(SUBDIRS:%=%/timestamp.txt)
CTIMESTAMPS=$(SUBDIRS:%=%/ctimestamp.txt)

all: $(TIMESTAMPS)

compare: $(CTIMESTAMPS)

%/timestamp.txt:
		cd $* && $(MAKE) -f $(PWD)/Makefile.model test

%/ctimestamp.txt:
		cd $* && $(MAKE) -f $(PWD)/Makefile.model compare

clean:
	for p in  $(SUBDIRS); do\
	( cd $$p && $(MAKE) -f $(PWD)/Makefile.model clean && cd ..);\
	done ;

resdir:
	for p in  $(SUBDIRS); do\
	( cd $$p && $(MAKE) -f $(PWD)/Makefile.model resdir && cd ..);\
	done ;

debug:
	@echo BENCHDIR=$(BENCHDIR)
	@echo SUBDIRS=$(SUBDIRS)

