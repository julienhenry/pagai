.SUFFIXES:

DEST=build
DEST_LINKS=.

CUDD_VERSION=2.4.2
YICES_VERSION=1.0.31
YICES_ARCH=i686-pc-linux-gnu-static-gmp
Z3_VERSION=2.19
LLVM_VERSION=2.9

all: install-cudd install-yices install-z3 install-llvm install-clang

install-cudd: $(DEST)/cudd-$(CUDD_VERSION)/cudd/libcudd.a $(DEST_LINKS)/cudd
install-yices: $(DEST)/yices-$(YICES_VERSION)/lib/libyices.a $(DEST_LINKS)/yices
install-z3: $(DEST)/z3-$(Z3_VERSION)/lib/libz3-gmp.so $(DEST_LINKS)/z3
install-llvm: $(DEST)/llvm-$(LLVM_VERSION)/install/lib/libLLVMBitReader.a $(DEST_LINKS)/llvm
install-clang: $(DEST)/llvm-$(LLVM_VERSION)/install/bin/clang

MKDIR_DEST=mkdir -p "$(DEST)"
DEST_ABS=${shell $(MKDIR_DEST) && cd $(DEST) && pwd}

# Symlinks
$(DEST_LINKS)/cudd: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/cudd-$(CUDD_VERSION) $@
$(DEST_LINKS)/yices: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/yices-$(YICES_VERSION) $@
$(DEST_LINKS)/z3: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/z3-$(Z3_VERSION) $@
$(DEST_LINKS)/llvm: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/llvm-$(LLVM_VERSION)/install $@


#### CUDD Installation
$(DEST)/cudd-$(CUDD_VERSION).tar.gz:
	$(MKDIR_DEST)
	curl -o "$@" "ftp://vlsi.colorado.edu/pub/cudd-$(CUDD_VERSION).tar.gz"

$(DEST)/cudd-$(CUDD_VERSION)/Makefile: $(DEST)/cudd-$(CUDD_VERSION).tar.gz cudd.patch
	cd $(DEST) && tar xzvf "cudd-$(CUDD_VERSION).tar.gz"
	(cd $(DEST)/"cudd-$(CUDD_VERSION)" && patch -p1) < ./cudd.patch

$(DEST)/cudd-$(CUDD_VERSION)/cudd/libcudd.a: $(DEST)/cudd-$(CUDD_VERSION)/Makefile
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && make
# call "make testobj" from obj, otherwise it tries using -O6 and fails
# to generate symbols for inline functions.
	cd $(DEST)/cudd-$(CUDD_VERSION)/obj && make testobj
# Just check that the build went OK, its fast
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && ./obj/testobj


#### Yices Installation
$(DEST)/yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz:
	$(MKDIR_DEST)
# TODO: find a clean way to have the user accept the licence
	curl -o "$@" "http://yices.csl.sri.com/cgi-bin/yices-newdownload.cgi?file=yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz&accept=I+accept"

$(DEST)/yices-$(YICES_VERSION)/lib/libyices.a: $(DEST)/yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz
	cd $(DEST) && tar xzvf "yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz"
# override the timestamp set by tar
	touch --no-create "$@"

#### Z3 Installation
$(DEST)/z3-$(Z3_VERSION).tar.gz:
	$(MKDIR_DEST)
	curl -L -o "$@" "http://research.microsoft.com/projects/z3/z3-$(Z3_VERSION).tar.gz"

$(DEST)/z3-$(Z3_VERSION)/lib/libz3-gmp.so: $(DEST)/z3-$(Z3_VERSION).tar.gz
	mkdir -p $(DEST)/z3-$(Z3_VERSION)
# z3-V.tar.gz expands itself into a directory without version but in
# case we need to experiment with different versions, force a
# directory name including a version number and use the same symlink
# trick as for others.
	cd $(DEST)/z3-$(Z3_VERSION) && tar --touch --strip-components=1 -xzvf ../z3-$(Z3_VERSION).tar.gz

#### LLVM Installation
CONFIGURE_LLVM=cd $(DEST)/llvm-$(LLVM_VERSION) && \
		./configure --prefix=$(DEST_ABS)/llvm-$(LLVM_VERSION)/install/

$(DEST)/llvm-$(LLVM_VERSION).tar.gz:
	$(MKDIR_DEST)
	curl -o "$@" "http://llvm.org/releases/$(LLVM_VERSION)/llvm-$(LLVM_VERSION).tgz"

$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac: $(DEST)/llvm-$(LLVM_VERSION).tar.gz
	cd $(DEST) && tar xzvf "llvm-$(LLVM_VERSION).tar.gz"
	touch --no-create "$@"

$(DEST)/llvm-$(LLVM_VERSION)/install/lib/libLLVMBitReader.a: $(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac
	$(CONFIGURE_LLVM)
	cd $(DEST)/llvm-$(LLVM_VERSION) && make && make install

# clang
$(DEST)/clang-$(LLVM_VERSION).tar.gz:
	$(MKDIR_DEST)
	curl -o "$@" 'http://llvm.org/releases/2.9/clang-2.9.tgz'

$(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt: $(DEST)/clang-$(LLVM_VERSION).tar.gz \
		$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac
	-$(RM) -r $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/
	cd $(DEST)/llvm-$(LLVM_VERSION)/tools/ && tar xzvf "$(DEST_ABS)/clang-$(LLVM_VERSION).tar.gz" && mv clang-2.9 clang
	touch --no-create "$@"

$(DEST)/llvm-$(LLVM_VERSION)/install/bin/clang: $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt
# The last configuration may have been done without clang as a subdirectory
# => rerun configure in case.
	$(CONFIGURE_LLVM)
	cd $(DEST)/llvm-$(LLVM_VERSION) && ./config.status --recheck && \
		make && make install

