#!/bin/bash

function usage () {
echo "
Usage:
./compile_llvm <OPTION> -i [FILE]

OPTION :
	-h        : help

	-o [FILE ]: name of the output file
	-p        : print the optimized llvm bytecode
	-g        : generate the .dot CFG
"
}

PRINT=0
GRAPH=0

while getopts “hpgi:o:” opt ; do
	case $opt in
		h)
			usage
			exit 1
            ;;
        p)
			PRINT=1
            ;;
		i)
			FILENAME=$OPTARG
			;;
		g)
			GRAPH=1
			;;
		o)
			OUTPUT=$OPTARG
			;;
        ?)
            usage
            exit
            ;;
     esac
done


BASENAME=`basename $FILENAME`
NAME=${BASENAME%%.*}
DIR=`dirname $FILENAME`

if [ -z $OUTPUT ] ; then 
	OUTPUT=${DIR}/${NAME}_opt.o
fi

clang -emit-llvm -c $FILENAME -o $DIR/$NAME.o
opt -mem2reg -loopsimplify -lowerswitch $DIR/$NAME.o -o $OUTPUT

if [ $GRAPH -eq 1 ] ; then
	opt -dot-cfg $OUTPUT
	mv *.dot $DIR
	dot -Tsvg -o $DIR/callgraph.svg $DIR/cfg.main.dot
fi

if [ $PRINT -eq 1 ] ; then
	opt -S $OUTPUT
fi
