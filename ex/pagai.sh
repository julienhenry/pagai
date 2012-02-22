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
	-s        : silent mode
	-c        : compare all techniques
	-M        : compare narrowing
	-G        : generate the .dot CFG
	-y        : use Yices instead of Microsoft Z3
	-b        : use the Bagnara Widening operator
	-d        : domain 1
	-D        : domain 2
	-p        : use the trunk version of pagai
	-t        : set a time limit (Pagai is killed after this time, default 800s)
"
}

PRINT=0
GRAPH=0
YICES=0
UNROLL=0
COMPARE=0
NCOMPARE=0
TIME_LIMIT=800
RESULT=
BITCODE=
PAGAI=Pagai
REQUIRED=0
SILENT=0
DOMAIN1=pk
DOMAIN2=pk

while getopts "hpygrbuGi:o:ct:O:spMd:D:" opt ; do
	case $opt in
		h)
			usage
			exit 1
            ;;
		i)
			FILENAME=$OPTARG
			REQUIRED=1
			;;
		d)
			DOMAIN1=$OPTARG
			;;
		D)
			DOMAIN2=$OPTARG
			;;
		u)
			UNROLL=1
			;;
		p)
			PAGAI=pagai
			;;
		c)
			COMPARE=1
			;;
		M)
			NCOMPARE=1
			;;
		G)
			GRAPH=1
			;;
		s)
			SILENT=1
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
		BITCODE=/tmp/${NAME}.bc
	else
		BITCODE=/tmp/${NAME}_unroll.bc
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

ulimit -t $TIME_LIMIT

ARGS="--domain $DOMAIN1 --domain2 $DOMAIN2 "

if [ -z $OUTPUT ] ; then 
	if [ $COMPARE -eq 1 ] ; then
		$PAGAI $ARGS -c -i $BITCODE
	elif [ $NCOMPARE -eq 1 ] ; then
		echo "$PAGAI $ARGS -t s -M -i $BITCODE"
		$PAGAI $ARGS -t s -M -i $BITCODE
	elif [ $YICES -eq 1 ] ; then
		$PAGAI $ARGS -y -i $BITCODE
	else
		$PAGAI $ARGS -i $BITCODE
	fi
else
	if [ $COMPARE -eq 1 ] ; then
			$PAGAI $ARGS -c -i $BITCODE -o $OUTPUT
	elif [ $NCOMPARE -eq 1 ] ; then
		$PAGAI $ARGS -t s -M -i $BITCODE -o $OUTPUT
	elif [ $YICES -eq 1 ] ; then
		$PAGAI $ARGS -y -i $BITCODE -o $OUTPUT
	else
		$PAGAI $ARGS -i $BITCODE -o $OUTPUT
	fi
fi
xs=$?

if [ $SILENT -eq 0 ] ; then
	case $xs in
	 0) echo "ok -- $NAME"
		exit 0
		 ;; # all fine
	 *) if [ $xs -gt 127 ]; then
	       echo "killed -- $NAME"
	    else
	       echo "error -- $NAME"
	    fi
		exit 0
	esac
fi

