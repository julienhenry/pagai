#!/bin/bash

function usage () {
echo "
Usage:
./analyzer-bc <OPTION> -i [FILE]

OPTIONS :
	-h        : help

	-o [FILE ]: name of the IR generated file
	-u        : unroll loops once
	-c        : compare all techniques
	-G        : generate the .dot CFG
	-y        : use Yices instead of Microsoft Z3
	-b        : use the Bagnara Widening operator
"
}

PRINT=0
GRAPH=0
YICES=0
UNROLL=0
COMPARE=0
RESULT=

REQUIRED=0

while getopts "hpygrbuGi:o:c" opt ; do
	case $opt in
		h)
			usage
			exit 1
            ;;
		i)
			FILENAME=$OPTARG
			REQUIRED=1
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

if [ $REQUIRED -eq 0 ] ; then
	usage
	exit
fi

BASENAME=`basename $FILENAME`
NAME=${BASENAME%%.*}
DIR=`dirname $FILENAME`

if [ -z $OUTPUT ] ; then 
	if [ $UNROLL -eq 0 ] ; then
		OUTPUT=/tmp/${NAME}_nounroll.bc
	else
		OUTPUT=/tmp/${NAME}.bc
	fi
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
		dot -Tsvg -o $i.svg $i
	done
fi

NAME=`basename $OUTPUT`
RESULT=/tmp/${NAME%%.*}.result
echo "running Pagai on $NAME"
if [ $COMPARE -eq 1 ] ; then
		pagai_2 -c -i $OUTPUT
else
	if [ $YICES -eq 1 ] ; then
		pagai_2 -y -i $OUTPUT
	else
		pagai_2 -i $OUTPUT
	fi
fi
