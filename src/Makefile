YICES_PATH	= /home/jhenry/local/yices
Z3_PATH		= /home/jhenry/local/z3
#LLVM_PATH	= /home/jhenry/local/llvm-2.9
LLVM_PATH	= /usr
CUDD_PATH	= /home/jhenry/local/cudd-2.4.2

SRC		= $(wildcard ../src/*.cc)
OBJ_REP		= ../obj/
SRC_REP		= ../src/
OBJS		= $(subst $(SRC_REP),$(OBJ_REP),$(SRC:.cc=.o))
CC			= clang++
#CPPFLAGS	= -Wall -g -pg #-O2 
CPPFLAGS	= -O2  
CPPFLAGS	= -Wall -g -DPRINT_DEBUG -O2 
#LDFLAGS		= -pg
LDFLAGS		= 
ICFLAGS		= -I$(YICES_PATH)/include -I$(Z3_PATH)/include -I$(LLVM_PATH)/include -I$(CUDD_PATH)/include 
#ICFLAGS		= 
EXEC		= analyzer

GMP_PREFIX=/usr
MPFR_PREFIX=/usr
APRON_PREFIX=/usr
PPL_PREFIX=/usr
CAMLIDL_PREFIX=/usr

CPPFLAGS += $(shell llvm-config --cxxflags jit bitreader bitwriter native) 
#CPPFLAGS += -I/import/linux/i686/soft/llvm/2.9/include  -DNDEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -O3 -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual

LDFLAGS  += $(shell llvm-config --ldflags --libs core jit bitreader bitwriter native)

CPPFLAGS += \
			-I$(GMP_PREFIX)/include \
			-I$(MPFR_PREFIX)/include \
			-I$(APRON_PREFIX)/include \
			-I$(CUDD_PATH)/include \


LCFLAGS += \
			-L$(GMP_PREFIX)/lib \
			-L$(MPFR_PREFIX)/lib \
			-L$(APRON_PREFIX)/lib \
			-L$(PPL_PREFIX)/lib \
			-L$(CAMLIDL_PREFIX)/lib/ocaml/apron \
			-L$(YICES_PATH)/lib \
			-L$(Z3_PATH)/lib \
			-L$(CUDD_PATH)/obj \
			-L$(CUDD_PATH)/mtr \
			-L$(CUDD_PATH)/st \
			-L$(CUDD_PATH)/epd \
			-L$(CUDD_PATH)/util \
			-L$(CUDD_PATH)/cudd \
			-Wl,-rpath $(Z3_PATH)/lib

#LDFLAGS += -static

LDFLAGS += -lpolkaMPQ -loctD -lboxMPFR -lap_ppl -lap_pkgrid -lapron 

LDFLAGS += -lppl

LDFLAGS += -lyices -lz3

LDFLAGS += -lmpfr -lgmpxx -lgmp

LDFLAGS += -ldl -lpthread -lutil

CUDD_LIBS = \
	$(CUDD_PATH)/obj/libobj.a \
	$(CUDD_PATH)/cudd/libcudd.a \
	$(CUDD_PATH)/mtr/libmtr.a \
	$(CUDD_PATH)/st/libst.a \
	$(CUDD_PATH)/util/libutil.a \
	$(CUDD_PATH)/epd/libepd.a


all: $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(LCFLAGS) -o $@ $^ $(CUDD_LIBS) $(LDFLAGS)

${OBJ_REP}%.o : %.cc *.h
	$(CC) $(ICFLAGS) -o $@ -c $< $(CPPFLAGS)

.PHONY: clean realclean

clean:
	rm -rf $(OBJ_REP)*.o

realclean: clean
	rm -rf $(EXEC)

