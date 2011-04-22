SRC		= $(wildcard ../src/*.cc)
BIN_REP		= ../bin/
SRC_REP		= ../src/
OBJS		= $(subst $(SRC_REP),$(BIN_REP),$(SRC:.cc=.o))
CC			= clang++
CPPFLAGS	= -Wall -g -DPRINT_DEBUG
LDFLAGS		=
ICFLAGS		= -I/net/cezanne/local/jhenry/yices/include -I/net/cezanne/local/jhenry/z3/include
#ICFLAGS		= 
EXEC		= analyzer

GMP_PREFIX=/usr
MPFR_PREFIX=/usr
APRON_PREFIX=/usr
PPL_PREFIX=/usr
CAMLIDL_PREFIX=/usr

CPPFLAGS += `llvm-config --cxxflags jit bitreader bitwriter native`

LDFLAGS  += `llvm-config --ldflags --libs core jit bitreader bitwriter native`

CPPFLAGS += \
			-I$(GMP_PREFIX)/include \
			-I$(MPFR_PREFIX)/include \
			-I$(APRON_PREFIX)/include \


LCFLAGS += \
			-L$(GMP_PREFIX)/lib \
			-L$(MPFR_PREFIX)/lib \
			-L$(APRON_PREFIX)/lib \
			-L$(PPL_PREFIX)/lib \
			-L$(CAMLIDL_PREFIX)/lib/ocaml/apron \
			-L/local/jhenry/yices/lib \
			-L/local/jhenry/z3/lib

LDFLAGS  += -lapron -lppl -lgmpxx \
			-lmpfr -lgmp  -lpolkaMPQ -loctD -lboxMPFR -lyices -lz3

all: $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(LCFLAGS) -o $@ $^ $(LDFLAGS)

${BIN_REP}%.o : %.cc *.h
	$(CC) $(ICFLAGS) -o $@ -c $< $(CPPFLAGS)

.PHONY: clean realclean

clean:
	rm -rf $(BIN_REP)*.o

realclean: clean
	rm -rf $(EXEC)

