#!/bin/bash

function usage () {
echo "
Usage:
./compile_llvm <OPTION> -i [FILE]

OPTION :
	-h        : help

	-o [FILE ]: name of the output file
	-p        : print the optimized llvm bytecode
"
}

while getopts “hpi:o:” opt ; do
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
		o)
			OUTPUT=$OPTARG
			;;
        ?)
            usage
            exit
            ;;
     esac
done

NAME=${FILENAME%%.*}

if [ -z $OUTPUT ] ; then 
	OUTPUT=${NAME}_opt.o
fi

clang -emit-llvm -c $FILENAME -o $NAME.o
opt -mem2reg $NAME.o -o $OUTPUT

if [ -n $PRINT ] ; then
	opt -S $OUTPUT
fi
