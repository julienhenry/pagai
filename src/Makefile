SRC		= $(wildcard ../src/*.cc)
BIN_REP		= ../bin/
SRC_REP		= ../src/
OBJS		= $(subst $(SRC_REP),$(BIN_REP),$(SRC:.cc=.o))
CC			= clang++
CPPFLAGS	= -Wall -g -DDEBUG
LDFLAGS		=
IDFLAGS		= -I/local/jhenry/llvm-2.8/include
EXEC		= analyzer


CPPFLAGS += `llvm-config --cxxflags jit bitreader bitwriter native`

LDFLAGS  += `llvm-config --ldflags --libs core jit bitreader bitwriter native`


all: $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(IDFLAGS) -o $@ $^ $(LDFLAGS)

${BIN_REP}%.o : %.cc *.h
	$(CC) $(IDFLAGS) -o $@ -c $< $(CPPFLAGS)

.PHONY: clean realclean

clean:
	rm -rf $(BIN_REP)*.o

realclean: clean
	rm -rf $(EXEC)

