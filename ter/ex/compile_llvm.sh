#!/bin/bash

FILENAME=$1
NAME=${FILENAME%%.*}


clang -emit-llvm -c $FILENAME
opt -mem2reg $NAME.o -o ${NAME}_opt.o

echo "Compilation finished"
