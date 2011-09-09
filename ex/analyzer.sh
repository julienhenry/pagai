#!/bin/bash

BASEDIR=$(dirname "$0")/..

function usage () {
echo "
Usage:
./analyzer <OPTION> -i [FILE]

OPTIONS :
	-h        : help

	-o [FILE ]: name of the IR generated file
	-p        : print the optimized llvm bytecode
	-u        : unroll loops once
	-G        : generate the .dot CFG
	-g        : use Lookahead Widening instead of Path Focusing
	-y        : use Yices instead of Microsoft Z3
	-c        : add compile options for clang
"
}

PRINT=0
GRAPH=0
GOPAN=0
YICES=0
UNROLL=0
COMPILE_OPTIONS=

while getopts "hpyguGi:o:c:" opt ; do
	case $opt in
		h)
			usage
			exit 1
            ;;
        p)
			PRINT=1
            ;;
		c)
			COMPILE_OPTIONS=$OPTARG
			;;
		i)
			FILENAME=$OPTARG
			;;
		u)
			UNROLL=1
			;;
		G)
			GRAPH=1
			;;
		y)
			YICES=1
			;;
		g)
			GOPAN=1
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
	echo "Please provide a filename (C source code) with -i FILE."
	exit 1
fi

BASENAME=`basename $FILENAME`
NAME=${BASENAME%%.*}
DIR=`dirname $FILENAME`

if [ -z "$OUTPUT" ] ; then 
	OUTPUT=/tmp/${NAME}.bc
fi

echo "Compilation using command-line:
clang -DNDEBUG -fno-exceptions $COMPILE_OPTIONS -emit-llvm -c $FILENAME -o $OUTPUT
"
clang -DNDEBUG -fno-exceptions $COMPILE_OPTIONS -emit-llvm -c "$FILENAME" -o "$OUTPUT"
#opt -mem2reg -loopsimplify -lowerswitch $OUTPUT -o $OUTPUT
if [ $UNROLL -eq 1 ] ; then
	opt -mem2reg -inline -lowerswitch -loops  -loop-simplify -loop-rotate -lcssa -loop-unroll -unroll-count=1 "$OUTPUT" -o "$OUTPUT"
else
	opt -mem2reg -inline -lowerswitch "$OUTPUT" -o "$OUTPUT"
fi

if [ $GRAPH -eq 1 ] ; then
	opt -dot-cfg "$OUTPUT" -o "$OUTPUT"
	mv *.dot $DIR
	for i in $DIR/*.dot ; do
		#dot -Tsvg -o $DIR/callgraph.svg $DIR/cfg.main.dot
		dot -Tsvg -o $i.svg $i
	done
fi

if [ $PRINT -eq 1 ] ; then
	opt -S $OUTPUT
fi

NAME=`basename $OUTPUT`
RESULT=/tmp/${NAME%%.*}.result
echo "running analyzer on $NAME"

if [ $GOPAN -eq 1 ] ; then
    "$BASEDIR"/src/analyzer -g -i "$OUTPUT"
elif [ $YICES -eq 1 ] ; then
    "$BASEDIR"/src/analyzer -y -i "$OUTPUT"
else
    "$BASEDIR"/src/analyzer -i "$OUTPUT"
fi

