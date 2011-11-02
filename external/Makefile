CUDD_VERSION=2.4.2
YICES_VERSION=1.0.31
YICES_ARCH=i686-pc-linux-gnu-static-gmp

all: install-cudd install-yices

install-cudd: cudd-$(CUDD_VERSION)/cudd/libcudd.a cudd
install-yices: yices-$(YICES_VERSION)/lib/libyices.a yices

cudd: $(MAKEFILE_LIST)
	ln -sf cudd-$(CUDD_VERSION) cudd
yices: $(MAKEFILE_LIST)
	ln -sf yices-$(YICES_VERSION) yices

cudd-$(CUDD_VERSION).tar.gz:
	curl -o "$@" "ftp://vlsi.colorado.edu/pub/cudd-$(CUDD_VERSION).tar.gz"

cudd-$(CUDD_VERSION)/Makefile: cudd-$(CUDD_VERSION).tar.gz cudd.patch
	tar xzvf "$<"
	cd "cudd-$(CUDD_VERSION)" && patch -p1 < ../cudd.patch

cudd-$(CUDD_VERSION)/cudd/libcudd.a: cudd-$(CUDD_VERSION)/Makefile
	cd cudd-$(CUDD_VERSION)/ && make
# call "make testobj" from obj, otherwise it tries using -O6 and fails
# to generate symbols for inline functions.
	cd cudd-$(CUDD_VERSION)/obj && make testobj
# Just check that the build went OK, its fast
	cd cudd-$(CUDD_VERSION)/ && ./obj/testobj

yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz:
# TODO: find a clean way to have the user accept the licence
	curl -o "$@" "http://yices.csl.sri.com/cgi-bin/yices-newdownload.cgi?file=yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz&accept=I+accept"

yices-$(YICES_VERSION)/lib/libyices.a: yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz
	tar xzvf "$<"
# override the timestamp set by tar
	touch "$@"
