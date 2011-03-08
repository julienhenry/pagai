SRC		= $(wildcard ../src/*.cc)
BIN_REP		= ../bin/
SRC_REP		= ../src/
OBJS		= $(subst $(SRC_REP),$(BIN_REP),$(SRC:.cc=.o))
CC			= clang++
CPPFLAGS	= -Wall -g -DDEBUG
LDFLAGS		=
#ICFLAGS		= -I/local/jhenry/llvm-2.8/include
ICFLAGS		= 
EXEC		= analyzer

GMP_PREFIX=/usr
MPFR_PREFIX=/net/cezanne/local/jhenry/apron_svn/install
APRON_PREFIX=/net/cezanne/local/jhenry/apron_svn/install
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
			-L$(CAMLIDL_PREFIX)/lib/ocaml/apron

LDFLAGS  += -lapron -lppl -lgmpxx \
			-lmpfr -lgmp  -lpolkaMPQ -loctD -lboxMPFR

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

