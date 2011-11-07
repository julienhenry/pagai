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

CUDD_ARCHIVE=cudd-$(CUDD_VERSION).tar.gz
YICES_ARCHIVE=yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz
Z3_ARCHIVE=z3-$(Z3_VERSION).tar.gz
LLVM_ARCHIVE=llvm-$(LLVM_VERSION).tar.gz
CLANG_ARCHIVE=clang-$(LLVM_VERSION).tgz

MKDIR_DEST=mkdir -p "$(DEST)"
DEST_ABS=${shell $(MKDIR_DEST) && cd $(DEST) && pwd}

# Small shell function to download using curl or wget if curl is not
# installed or doesn't work.
DOWNLOAD=download () { \
	url=$$1 ;\
	output=$$2 ;\
	curl -L --fail -o "$$output" "$$url" || \
	wget "$$url" -O "$$output" || { \
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
	ln -sf $(DEST_ABS)/z3-$(Z3_VERSION) $@
$(DEST_LINKS)/llvm: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/llvm-$(LLVM_VERSION)/install $@


#### CUDD Installation
$(DEST)/$(CUDD_ARCHIVE):
	$(MKDIR_DEST)
	$(DOWNLOAD) "ftp://vlsi.colorado.edu/pub/$(CUDD_ARCHIVE)" "$@"

$(DEST)/cudd-$(CUDD_VERSION)/Makefile: $(DEST)/$(CUDD_ARCHIVE) cudd.patch
	cd $(DEST) && tar xzvf "$(CUDD_ARCHIVE)"
	(cd $(DEST)/"cudd-$(CUDD_VERSION)" && patch -p1) < ./cudd.patch

$(DEST)/cudd-$(CUDD_VERSION)/cudd/libcudd.a: $(DEST)/cudd-$(CUDD_VERSION)/Makefile
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && make
# call "make testobj" from obj, otherwise it tries using -O6 and fails
# to generate symbols for inline functions.
	cd $(DEST)/cudd-$(CUDD_VERSION)/obj && make testobj
# Just check that the build went OK, its fast
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && ./obj/testobj


#### Yices Installation
$(DEST)/$(YICES_ARCHIVE):
	$(MKDIR_DEST)
# TODO: find a clean way to have the user accept the licence
	$(DOWNLOAD) "http://yices.csl.sri.com/cgi-bin/yices-newdownload.cgi?file=$(YICES_ARCHIVE)&accept=I+accept" "$@"

$(DEST)/yices-$(YICES_VERSION)/lib/libyices.a: $(DEST)/$(YICES_ARCHIVE)
	cd $(DEST) && tar xzvf "$(YICES_ARCHIVE)"
# override the timestamp set by tar
	touch --no-create "$@"

#### Z3 Installation
$(DEST)/$(Z3_ARCHIVE):
	$(MKDIR_DEST)
	$(DOWNLOAD) "http://research.microsoft.com/projects/z3/$(Z3_ARCHIVE)" "$@"

$(DEST)/z3-$(Z3_VERSION)/lib/libz3-gmp.so: $(DEST)/$(Z3_ARCHIVE)
	mkdir -p $(DEST)/z3-$(Z3_VERSION)
# z3-V.tar.gz expands itself into a directory without version but in
# case we need to experiment with different versions, force a
# directory name including a version number and use the same symlink
# trick as for others.
	cd $(DEST)/z3-$(Z3_VERSION) && tar --touch --strip-components=1 -xzvf ../$(Z3_ARCHIVE)

#### LLVM Installation
CONFIGURE_LLVM=cd $(DEST)/llvm-$(LLVM_VERSION) && \
		./configure --prefix=$(DEST_ABS)/llvm-$(LLVM_VERSION)/install/

$(DEST)/$(LLVM_ARCHIVE):
	$(MKDIR_DEST)
	$(DOWNLOAD) "http://llvm.org/releases/$(LLVM_VERSION)/llvm-$(LLVM_VERSION).tgz" "$@"

$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac: $(DEST)/$(LLVM_ARCHIVE)
	cd $(DEST) && tar xzvf "$(LLVM_ARCHIVE)"
	touch --no-create "$@"

$(DEST)/llvm-$(LLVM_VERSION)/install/lib/libLLVMBitReader.a: $(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac
	$(CONFIGURE_LLVM)
	cd $(DEST)/llvm-$(LLVM_VERSION) && make && make install

# clang
$(DEST)/$(CLANG_ARCHIVE):
	$(MKDIR_DEST)
	$(DOWNLOAD) 'http://llvm.org/releases/$(LLVM_VERSION)/$(CLANG_ARCHIVE)' "$@"

$(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt: $(DEST)/$(CLANG_ARCHIVE) \
		$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac
	-$(RM) -r $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/
	cd $(DEST)/llvm-$(LLVM_VERSION)/tools/ && tar xzvf "$(DEST_ABS)/$(CLANG_ARCHIVE)" && mv clang-$(LLVM_VERSION) clang
	touch --no-create "$@"

$(DEST)/llvm-$(LLVM_VERSION)/install/bin/clang: $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt
# The last configuration may have been done without clang as a subdirectory
# => rerun configure in case.
	$(CONFIGURE_LLVM)
	cd $(DEST)/llvm-$(LLVM_VERSION) && ./config.status --recheck && \
		make && make install

