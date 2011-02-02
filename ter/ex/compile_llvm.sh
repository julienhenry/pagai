#!/bin/bash

FILENAME=$1
NAME=${FILENAME%%.*}


llvm-clang -emit-llvm -c $FILENAME
opt-2.7 -mem2reg $NAME.o -o ${NAME}_opt.o

echo "Compilation finished"
