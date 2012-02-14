#!/bin/bash

function usage () {
echo "
Usage:
./pagai.sh <OPTION> -i [FILE]

OPTIONS :
	-h        : help

	-O [FILE ]: name of the IR generated file
	-o [FILE ]: output file
	-u        : unroll loops once
	-c        : compare all techniques
	-G        : generate the .dot CFG
	-y        : use Yices instead of Microsoft Z3
	-b        : use the Bagnara Widening operator
	-t        : set a time limit (Pagai is killed after this time, default 800s)
"
}

PRINT=0
GRAPH=0
YICES=0
UNROLL=0
COMPARE=0
TIME_LIMIT=800
RESULT=
BITCODE=

REQUIRED=0

while getopts "hpygrbuGi:o:ct:O:" opt ; do
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
		t)
			TIME_LIMIT=$OPTARG
			;;
		o)
			OUTPUT=$OPTARG
			;;
		O)
			BITCODE=$OPTARG
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

if [ -z $BITCODE ] ; then 
	if [ $UNROLL -eq 0 ] ; then
		BITCODE=/tmp/${NAME}_nounroll.bc
	else
		BITCODE=/tmp/${NAME}.bc
	fi
fi

if [ $UNROLL -eq 1 ] ; then
	opt -mem2reg -inline -lowerswitch -loops  -loop-simplify -loop-rotate -lcssa -loop-unroll -unroll-count=1 $FILENAME -o $BITCODE
else
	opt -mem2reg -inline -lowerswitch $FILENAME -o $BITCODE
fi

if [ $GRAPH -eq 1 ] ; then
	opt -dot-cfg $BITCODE -o $BITCODE
	mv *.dot $DIR
	for i in `ls $DIR/*.dot` ; do
		dot -Tsvg -o $i.svg $i
	done
fi

NAME=`basename $BITCODE`
RESULT=/tmp/${NAME%%.*}.result

echo "running Pagai on $NAME"

ulimit -t $TIME_LIMIT

if [ -z $OUTPUT ] ; then 
	if [ $COMPARE -eq 1 ] ; then
			pagai -c -i $BITCODE
	else
		if [ $YICES -eq 1 ] ; then
			pagai -y -i $BITCODE
		else
			pagai -i $BITCODE
		fi
	fi
else
	if [ $COMPARE -eq 1 ] ; then
			pagai -c -i $BITCODE -o $OUTPUT
	else
		if [ $YICES -eq 1 ] ; then
			pagai -y -i $BITCODE -o $OUTPUT
		else
			pagai -i $BITCODE -o $OUTPUT
		fi
	fi
fi
xs=$?
case $xs in
 0) echo "ok"
	exit 0
	 ;; # all fine
 *) if [ $xs -gt 127 ]; then
       echo "killed"
    else
       echo "internal error"
    fi
	exit 1
esac

