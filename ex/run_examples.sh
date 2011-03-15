#!/bin/bash

OLD_DIR=`pwd`
cd /home/jhenry/m2r/ex

echo "compilation of the examples..."
for FILENAME in `ls *.{c,cpp}` ; do
	BASENAME=`basename $FILENAME`
	NAME=${BASENAME%%.*}

	./compile_llvm.sh -i $FILENAME -o bin/$NAME.o
done

for FILENAME in `ls bin/*.o bin/*.bc` ; do
	NAME=`basename $FILENAME`
	RESULT=results/${NAME%%.*}.result
	echo "running $NAME"
	../src/analyzer $FILENAME > $RESULT
done



cd $OLD_DIR
