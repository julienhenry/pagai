#!/bin/bash

function usage () {
echo "
Usage:
./analyzer <OPTION> -i [FILE]

OPTIONS :
	-h        : help

	-o [FILE ]: name of the IR generated file
	-p        : print the optimized llvm bytecode
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
COMPILE_OPTIONS=

while getopts “hpygGi:o:c:” opt ; do
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


BASENAME=`basename $FILENAME`
NAME=${BASENAME%%.*}
DIR=`dirname $FILENAME`

if [ -z $OUTPUT ] ; then 
	OUTPUT=/tmp/${NAME}.bc
fi

echo "Compilation using command-line:
clang -DNDEBUG -fno-exceptions $COMPILE_OPTIONS -emit-llvm -c $FILENAME -o $OUTPUT
"
clang -DNDEBUG -fno-exceptions $COMPILE_OPTIONS -emit-llvm -c $FILENAME -o $OUTPUT
#opt -mem2reg -loopsimplify -lowerswitch $OUTPUT -o $OUTPUT
opt -mem2reg -lowerswitch $OUTPUT -o $OUTPUT

if [ $GRAPH -eq 1 ] ; then
	opt -dot-cfg $OUTPUT -o $OUTPUT
	mv *.dot $DIR
	for i in `ls $DIR/*.dot` ; do
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
	/home/jhenry/m2r/src/analyzer -g -i $OUTPUT
else
	if [ $YICES -eq 1 ] ; then
		/home/jhenry/m2r/src/analyzer -y -i $OUTPUT
	else
		/home/jhenry/m2r/src/analyzer -i $OUTPUT
	fi
fi

