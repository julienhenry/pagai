#!/bin/bash

function usage () {
echo "
Usage:
./compile_llvm.sh <OPTION> -i [FILE]

OPTION :
	-h        : help

	-o [FILE ]: name of the output file
	-p        : print the llvm bytecode
	-g        : generate the .dot CFG
	-O        : apply optimisations to the bytecode
"
}

PRINT=0
GRAPH=0
OPT=0

while getopts "hpgi:o:O" opt ; do
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
		O)
			OPT=1
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

if [ -z "$FILENAME" ]; then
	echo "Please provide a filename with -i FILE."
	exit 1
fi

BASENAME=`basename $FILENAME`
NAME=${BASENAME%%.*}
DIR=`dirname $FILENAME`

if [ -z "$OUTPUT" ] ; then 
	OUTPUT=${DIR}/${NAME}.bc
fi

clang -emit-llvm -c $FILENAME -o $OUTPUT
if [ $OPT -eq 1 ] ; then
	opt -mem2reg -inline -lowerswitch -loops  -loop-simplify -loop-rotate -lcssa -loop-unroll -unroll-count=1 $OUTPUT -o $OUTPUT
else
	opt -mem2reg -lowerswitch -inline $OUTPUT -o $OUTPUT
fi

if [ $GRAPH -eq 1 ] ; then
	opt -dot-cfg $OUTPUT -o $OUTPUT
	mv *.dot $DIR
	for i in `ls $DIR/*.dot` ; do
		dot -Tsvg -o $i.svg $i
	done
fi

if [ $PRINT -eq 1 ] ; then
	opt -S $OUTPUT
fi
