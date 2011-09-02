#!/bin/bash

function usage () {
echo "
Usage:
./analyzer-bc <OPTION> -i [FILE]

OPTIONS :
	-h        : help

	-o [FILE ]: name of the IR generated file
	-p        : print the optimized llvm bytecode
	-u        : unroll loops once
	-c        : compare both techniques
	-G        : generate the .dot CFG
	-g        : use Lookahead Widening instead of Path Focusing
	-y        : use Yices instead of Microsoft Z3
"
}

PRINT=0
GRAPH=0
GOPAN=0
YICES=0
UNROLL=0
COMPARE=0
RESULT=

while getopts “hpygruGi:o:c” opt ; do
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
		u)
			UNROLL=1
			;;
		c)
			COMPARE=1
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
		r)
			RESULT=$OPTARG
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

if [ $UNROLL -eq 1 ] ; then
	opt -mem2reg -inline -lowerswitch -loops  -loop-simplify -loop-rotate -lcssa -loop-unroll -unroll-count=1 $FILENAME -o $OUTPUT
else
	opt -mem2reg -inline -lowerswitch $FILENAME -o $OUTPUT
fi

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
if [ $COMPARE -eq 1 ] ; then
	analyzer_2 -c -b -i $OUTPUT
else
	if [ $GOPAN -eq 1 ] ; then
		analyzer_2 -g -b -i $OUTPUT
	else
		if [ $YICES -eq 1 ] ; then
			analyzer_2 -y -b -i $OUTPUT
		else
			analyzer_2 -b -i $OUTPUT
		fi
	fi
fi
