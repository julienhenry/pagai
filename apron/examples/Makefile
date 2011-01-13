
GMP_PREFIX=/usr
MPFR_PREFIX=/usr
APRON_PREFIX=/usr
PPL_PREFIX=/usr
CAMLIDL_PREFIX=/usr

CC=clang++
CXX=clang++

ICFLAGS = \
-I$(GMP_PREFIX)/include \
-I$(MPFR_PREFIX)/include \
-I$(APRON_PREFIX)/include \
-Wall -W -Wstrict-prototypes -Wimplicit-function-declaration \
 -Winline -Wconversion -pedantic-errors

LCFLAGS = \
-L$(GMP_PREFIX)/lib \
-L$(MPFR_PREFIX)/lib \
-L$(APRON_PREFIX)/lib \
-L$(PPL_PREFIX)/lib \
-L$(CAMLIDL_PREFIX)/lib/ocaml/apron

all: myexampleg wideningg

%g: %g.o
	$(CXX) $(CXXFLAGS) $(ICFLAGS) $(LCFLAGS) -o $@  $<  -lapron -lppl -lgmpxx -lmpfr -lgmp -lap_ppl -lpolkaMPQ -loctD -lboxMPFR

%g.o: %.cpp
	$(CC) $(CFLAGS_DEBUG) $(ICFLAGS) -c -o $@ $<

clean:
	rm myexampleg
