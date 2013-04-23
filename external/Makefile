.SUFFIXES:

DEST=build
DEST_LINKS=.

CUDD_VERSION=2.5.0
YICES_VERSION=1.0.37
YICES_ARCH=i686-pc-linux-gnu-static-gmp
Z3_VERSION=2012-12-21
LLVM_VERSION=3.1
APRON_VERSION=2012-11-19
PPL_VERSION=1.0
CUDD_XCFLAGS=

ARCH=${shell uname -m | sed -e 's/i.86/iX86/'}
ifeq ($(ARCH), x86_64)
YICES_ARCH=x86_64-pc-linux-gnu-static-gmp
Z3_ARCH=-x64
CUDD_XCFLAGS='XCFLAGS=-mtune=native -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8'
endif

OS=${shell uname}
#ifeq ($(OS), Darwin)
#TAR=gnutar
#else
TAR=tar
#endif

GMP_PREFIX=/usr

.PHONY: all install-cudd install-z3 install-llvm install-clang install-apron install-ppl
all: install-cudd install-z3 install-llvm install-clang install-apron install-ppl

ifeq ($(OS), Darwin)
SOEXTENSION=.dylib
else
ifeq ($(OS), Cygwin)
SOEXTENSION=.dll
else
SOEXTENSION=.so
endif
endif

CUDD_COMPILED=$(DEST)/cudd-$(CUDD_VERSION)/cudd/libcudd.a
YICES_COMPILED=$(DEST)/yices-$(YICES_VERSION)/lib/libyices.a
Z3_COMPILED=$(DEST)/z3_$(Z3_VERSION)/install/lib/libz3$(SOEXTENSION)
LLVM_COMPILED=$(DEST)/llvm-$(LLVM_VERSION)/install/lib/libLLVMBitReader.a
CLANG_COMPILED=$(DEST)/llvm-$(LLVM_VERSION)/install/bin/clang
APRON_COMPILED=$(DEST)/apron-$(APRON_VERSION)/install/lib/libapron$(SOEXTENSION)
PPL_COMPILED=$(DEST)/ppl-$(PPL_VERSION)/installdir/lib/libppl$(SOEXTENSION)

install-cudd: $(CUDD_COMPILED) $(DEST_LINKS)/cudd
install-yices: $(YICES_COMPILED) $(DEST_LINKS)/yices
install-z3: $(Z3_COMPILED) $(DEST_LINKS)/z3
install-llvm: $(LLVM_COMPILED) $(DEST_LINKS)/llvm
install-clang: $(CLANG_COMPILED) $(DEST_LINKS)/llvm
install-apron: $(APRON_COMPILED) $(DEST_LINKS)/apron
install-ppl: $(PPL_COMPILED) $(DEST_LINKS)/ppl

CUDD_ARCHIVE=cudd-$(CUDD_VERSION).tar.gz
YICES_ARCHIVE=yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz
Z3_ARCHIVE=z3_$(Z3_VERSION).tar.bz2
LLVM_ARCHIVE=llvm-$(LLVM_VERSION).src.tar.gz
CLANG_ARCHIVE=clang-$(LLVM_VERSION).src.tar.gz
APRON_ARCHIVE=apron_$(APRON_VERSION).tar.gz
PPL_ARCHIVE=ppl-$(PPL_VERSION).tar.gz

DEST_ABS=${shell mkdir -p "$(DEST)" && cd $(DEST) && pwd}

# Small shell function to download using curl or wget if curl is not
# installed or doesn't work.
DOWNLOAD=download () { \
	url=$$1 ;\
	output=$$2 ;\
	mkdir -p "$$(dirname "$$output")" ;\
	curl -L --fail -o "$$output" "$$url" || \
	wget "$$url" -O "$$output" || { \
		$(RM) "$$output" ;\
		echo "Neither curl nor wget could get the document, sorry." ;\
		exit 1 ;\
	} ;\
}; download


# Symlinks
$(DEST_LINKS)/cudd: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/cudd-$(CUDD_VERSION) $@
$(DEST_LINKS)/yices: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/yices-$(YICES_VERSION) $@
$(DEST_LINKS)/z3: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/z3_$(Z3_VERSION)/install $@
$(DEST_LINKS)/llvm: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/llvm-$(LLVM_VERSION)/install $@
$(DEST_LINKS)/apron: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/apron-$(APRON_VERSION)/install $@
$(DEST_LINKS)/ppl: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/ppl-$(PPL_VERSION)/installdir $@


#### CUDD Installation
$(DEST)/$(CUDD_ARCHIVE):
	$(DOWNLOAD) "ftp://vlsi.colorado.edu/pub/$(CUDD_ARCHIVE)" "$@"

$(DEST)/cudd-$(CUDD_VERSION)/cudd/cudd.h Â$(DEST)/cudd-$(CUDD_VERSION)/Makefile: $(DEST)/$(CUDD_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(CUDD_ARCHIVE)" && cd cudd-$(CUDD_VERSION) && patch -p1 < ../../cudd-$(CUDD_VERSION).patch
	touch -c $@

$(CUDD_COMPILED): $(DEST)/cudd-$(CUDD_VERSION)/cudd/cudd.h
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && $(MAKE) $(CUDD_XCFLAGS)
# call "make testobj" from obj, otherwise it tries using -O6 and fails
# to generate symbols for inline functions.
	cd $(DEST)/cudd-$(CUDD_VERSION)/obj && $(MAKE) $(CUDD_XCFLAGS) testobj
# Just check that the build went OK, its fast
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && ./obj/testobj


#### Yices Installation
$(DEST)/$(YICES_ARCHIVE):
# TODO: find a clean way to have the user accept the licence
	$(DOWNLOAD) "http://yices.csl.sri.com/cgi-bin/yices-newdownload.cgi?file=$(YICES_ARCHIVE)&accept=I+accept" "$@"

$(YICES_COMPILED): $(DEST)/$(YICES_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(YICES_ARCHIVE)"
# override the timestamp set by tar
	touch -c "$@"

#### Z3 Installation
$(DEST)/$(Z3_ARCHIVE):
	$(DOWNLOAD) "http://forge.imag.fr/frs/download.php/343/$(Z3_ARCHIVE)" "$@"

$(DEST)/z3_$(Z3_VERSION)/README: $(DEST)/$(Z3_ARCHIVE)
	tar -x -C $(DEST) -jvf $(DEST)/$(Z3_ARCHIVE)
	rm -rf $(DEST)/z3_$(Z3_VERSION)
	mv $(DEST)/z3 $(DEST)/z3_$(Z3_VERSION)
	touch -c $@

$(Z3_COMPILED): $(DEST)/z3_$(Z3_VERSION)/README
	cd $(DEST)/z3_$(Z3_VERSION)/ && autoconf && \
		./configure \
			--prefix $(DEST_ABS)/z3_$(Z3_VERSION)/install/ && \
		python scripts/mk_make.py && \
		sed -i -e 's@.*python.*dist-packages.*@@' build/Makefile && \
		cd build && \
		$(MAKE)
	- cd $(DEST)/z3_$(Z3_VERSION)/build && $(MAKE) install

#### LLVM + Clang Installation
$(DEST)/$(LLVM_ARCHIVE):
	$(DOWNLOAD) "http://llvm.org/releases/$(LLVM_VERSION)/$(LLVM_ARCHIVE)" "$@"

$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac: $(DEST)/$(LLVM_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(LLVM_ARCHIVE)" && mv llvm-$(LLVM_VERSION).src llvm-$(LLVM_VERSION)
	touch -c "$@"

$(LLVM_COMPILED) $(CLANG_COMPILED): $(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt
	cd $(DEST)/llvm-$(LLVM_VERSION) && \
		./configure --prefix=$(DEST_ABS)/llvm-$(LLVM_VERSION)/install && \
		$(MAKE) && $(MAKE) install

# clang
$(DEST)/$(CLANG_ARCHIVE):
	$(DOWNLOAD) 'http://llvm.org/releases/$(LLVM_VERSION)/$(CLANG_ARCHIVE)' "$@"

$(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt: $(DEST)/$(CLANG_ARCHIVE) \
		$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac
	-$(RM) -r $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/
	cd $(DEST)/llvm-$(LLVM_VERSION)/tools/ && $(TAR) xzvf "$(DEST_ABS)/$(CLANG_ARCHIVE)" && mv clang-$(LLVM_VERSION).src clang
	touch -c "$@"

#### Apron
$(DEST)/$(APRON_ARCHIVE):
	$(DOWNLOAD) 'http://forge.imag.fr/frs/download.php/333/$(APRON_ARCHIVE)' "$@"

$(DEST)/apron-$(APRON_VERSION)/apron/README: $(DEST)/$(APRON_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(DEST_ABS)/$(APRON_ARCHIVE)" && mv apron apron-$(APRON_VERSION)
	cd $(DEST)/apron-$(APRON_VERSION) && patch -p4 < ../../apron-2012-11-19.patch && patch -p0 < ../../apron-2012-11-19-environment.patch
	touch -c "$@"

APRON_HAS_SHARED=1
ifeq ($(OS), Darwin)
APRON_HAS_SHARED=
else
ifeq ($(OS), Cygwin)
APRON_HAS_SHARED=
endif
endif

# -e 's?^ *\(#\|\) *HAS_OCAML *=.*$$?HAS_OCAML =?'
# -e 's?^ *\(#\|\) *HAS_OCAMLOPT *=.*$$?HAS_OCAMLOPT =?'

# DM Trying to use ocamlfind when unavailable causes numerous problems.
$(APRON_COMPILED): $(DEST)/apron-$(APRON_VERSION)/apron/README $(PPL_COMPILED)
	sed -e 's?^\(APRON_PREFIX *= *\).*$$?\1 $(DEST_ABS)/apron-$(APRON_VERSION)/install?' \
	    -e 's?^\(PPL_PREFIX *= *\).*$$?\1 $(DEST_ABS)/ppl-$(PPL_VERSION)/installdir?' \
	    -e 's?^\(MLGMPIDL_PREFIX *= *\).*$$?\1 $(DEST_ABS)/apron-$(APRON_VERSION)/install?' \
	    -e 's?^ *\(#\|\) *HAS_PPL *=.*$$?HAS_PPL = 1?' \
		-e 's/^OCAMLFIND *=.*/OCAMLFIND =/' \
		-e 's/^HAS_SHARED *=.*/HAS_SHARED = $(APRON_HAS_SHARED)/' \
		-e 's/^HAS_OCAML *=.*/HAS_OCAML =/' \
		-e 's/^HAS_OCAMLOPT *=.*/HAS_OCAMLOPT =/' \
		-e 's/^# *HAS_PPL *=.*/HAS_PPL = 1/' \
		< $(DEST)/apron-$(APRON_VERSION)/Makefile.config.model \
		> $(DEST)/apron-$(APRON_VERSION)/Makefile.config
	cd $(DEST)/apron-$(APRON_VERSION) && $(MAKE) && $(MAKE) install		

#### PPL Library
$(DEST)/$(PPL_ARCHIVE):
	$(DOWNLOAD) 'ftp://ftp.cs.unipr.it/pub/ppl/releases/$(PPL_VERSION)/$(PPL_ARCHIVE)' "$@"

$(DEST)/ppl-$(PPL_VERSION)/README: $(DEST)/$(PPL_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(DEST_ABS)/$(PPL_ARCHIVE)"
	touch -c "$@"

$(PPL_COMPILED): $(DEST)/ppl-$(PPL_VERSION)/README
	cd $(DEST)/ppl-$(PPL_VERSION)/ && \
		./configure \
			--prefix $(DEST_ABS)/ppl-$(PPL_VERSION)/installdir/ \
			--with-gmp=$(GMP_PREFIX) \
			--enable-interfaces=c,c++ && \
		$(MAKE) && $(MAKE) install
