CUDD_VERSION=2.4.2
YICES_VERSION=1.0.31
YICES_ARCH=i686-pc-linux-gnu-static-gmp
Z3_VERSION=2.19
LLVM_VERSION=2.9

all: install-cudd install-yices install-z3 install-llvm

install-cudd: cudd-$(CUDD_VERSION)/cudd/libcudd.a cudd
install-yices: yices-$(YICES_VERSION)/lib/libyices.a yices
install-z3: z3-$(Z3_VERSION)/lib/libz3-gmp.so z3
install-llvm: llvm-$(LLVM_VERSION)/install/lib/libLLVMBitReader.a llvm

# Symlinks
cudd: $(MAKEFILE_LIST)
	ln -sf cudd-$(CUDD_VERSION) cudd
yices: $(MAKEFILE_LIST)
	ln -sf yices-$(YICES_VERSION) yices
z3: $(MAKEFILE_LIST)
	ln -sf z3-$(Z3_VERSION) z3
llvm: $(MAKEFILE_LIST)
	ln -sf llvm-$(LLVM_VERSION)/install llvm


#### CUDD Installation
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


#### Yices Installation
yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz:
# TODO: find a clean way to have the user accept the licence
	curl -o "$@" "http://yices.csl.sri.com/cgi-bin/yices-newdownload.cgi?file=yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz&accept=I+accept"

yices-$(YICES_VERSION)/lib/libyices.a: yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz
	tar xzvf "$<"
# override the timestamp set by tar
	touch --no-create "$@"

#### Z3 Installation
z3-$(Z3_VERSION).tar.gz:
	curl -L -o "$@" "http://research.microsoft.com/projects/z3/z3-$(Z3_VERSION).tar.gz"

z3-$(Z3_VERSION)/lib/libz3-gmp.so:
	mkdir -p z3-$(Z3_VERSION)
# z3-V.tar.gz expands itself into a directory without version but in
# case we need to experiment with different versions, force a
# directory name including a version number and use the same symlink
# trick as for others.
	cd z3-$(Z3_VERSION) && tar --touch --strip-components=1 -xzvf ../z3-$(Z3_VERSION).tar.gz

#### LLVM Installation
llvm-$(LLVM_VERSION).tar.gz:
	curl -o "$@" "http://llvm.org/releases/$(LLVM_VERSION)/llvm-$(LLVM_VERSION).tgz"

llvm-$(LLVM_VERSION)/README.txt: llvm-$(LLVM_VERSION).tar.gz
	tar xzvf "$<"
	touch --no-create "$@"

llvm-$(LLVM_VERSION)/install/lib/libLLVMBitReader.a: llvm-$(LLVM_VERSION)/README.txt
	cd llvm-$(LLVM_VERSION) && \
		./configure --prefix=$(PWD)/llvm-$(LLVM_VERSION)/install/ && \
		make && make install
