SRC		= $(wildcard ../new/*.cc)
BIN_REP		= ../bin/
SRC_REP		= ../new/
OBJS		= $(subst $(SRC_REP),$(BIN_REP),$(SRC:.cc=.o))
CC		= clang++
CPPFLAGS	= -W -Wall -g
LDFLAGS		=
EXEC		= llvm2smt


CPPFLAGS += `llvm-config-2.8 --cxxflags jit bitreader bitwriter native`

LDFLAGS  += `llvm-config-2.8 --ldflags --libs jit bitreader bitwriter native`

all: $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

${BIN_REP}%.o : %.cc *.h
	$(CC) -o $@ -c $< $(CPPFLAGS)

.PHONY: clean realclean

clean:
	rm -rf $(BIN_REP)*.o

realclean: clean
	rm -rf $(EXEC)

