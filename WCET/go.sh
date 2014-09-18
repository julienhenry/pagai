#!/bin/bash

SCRIPTDIR=`dirname $0`

export LD_LIBRARY_PATH=$SCRIPTDIR/../external/z3/lib/:$LD_LIBRARY_PATH
export PAGAI_PATH=$SCRIPTDIR/../src/

function usage () {
echo "
Usage:
./go.sh <OPTION> -c [FILE]

OPTION :
	-h        : help
	-c <FILE> : C input file
	-M <FILE> : matching file
	-S <FILE> : longest syntactic path file
	-b        : FILE given by option -c is already a bitcode FILE
	-s        : do not add summaries in the SMT formula
	-l        : avoid non-linear arithmetic in the SMT formula
	-d        : create .dot file
	-n        : skip bitcode optimisations
	-I        : with input functions when running charcute_bitcode
	-m        : print model of the longest feasible path
	-T <int>  : set the ulimit -t time (default:120)
	-R        : Recursive cuts
"
}

SUMMARIES=1
NONLINEAR=1
DOT=0
SKIPCLANG=0
SKIPOPTIM=0
PRINTMODEL=""
DOMATCHING=0
PRINTSYNTACTIC=0
RECURSIVECUTS=0
WITHINPUTFUNCTION=""
MATCHINGFILE=""
ULIMIT=120

while getopts “hc:lsdbmnM:S:IT:R” opt ; do
	case $opt in
		h)
			usage
			exit 1
            ;;
		c)
			FILE=$OPTARG
			;;
		M)
            DOMATCHING=1
			MATCHINGFILE=$OPTARG
			;;
		S)
            PRINTSYNTACTIC=1
            LONGESTSYNTACTICFILE=$OPTARG
			;;
		T)
            ULIMIT=$OPTARG
			;;
		s)
			SUMMARIES=0
			;;
		l)
			NONLINEAR=0
			;;
		d)
			DOT=1
			;;
		b)
			SKIPCLANG=1
			;;
		I)
			# not required anymore 
			WITHINPUTFUNCTION=""
			;;
		n)
			SKIPOPTIM=1
			;;
		m)
			PRINTMODEL="-m"
			;;
		R) 
			RECURSIVECUTS=1
			;;
        ?)
            usage
            exit
            ;;
     esac
done

if [ -z $FILE ] ; then 
	usage
	exit
fi

GREEN="\\033[1;32m"
NORMAL="\\033[0;39m"
RED="\\033[1;31m"
PINK="\\033[1;35m"
BLUE="\\033[1;34m"
WHITE="\\033[0;02m"
WHITE2="\\033[1;08m"
YELLOW="\\033[1;33m"
CYAN="\\033[1;36m"

dir=`dirname $FILE`
filename=`basename $FILE`
name=${filename%.*}

function ccompile {
	echo "Compiling..."
	echo "dir = " $dir
	clang -emit-llvm -c $FILE -o $dir/$name.bc
}

function bcoptimize {
	$PAGAI_PATH/pagai -i $dir/$name.bc --dump-ll --wcet --loop-unroll > $dir/$name.ll
	llvm-as $dir/$name.ll
}

function gensmtformula {
	if [ $NONLINEAR -eq 0 ] ; then
		echo "$PAGAI_PATH/pagai -i $dir/$name.bc --printformula --skipnonlinear > $dir/$name.gen"
		$PAGAI_PATH/pagai -i $dir/$name.bc -s z3 --wcet --printformula --skipnonlinear > $dir/$name.gen
	else
		$PAGAI_PATH/pagai -i $dir/$name.bc -s z3 --wcet --printformula > $dir/$name.gen
	fi
}

if [ $SKIPCLANG -eq 0 ] ; then
	ccompile
fi
if [ $SKIPOPTIM -eq 0 ] ; then
	bcoptimize
fi

if [ $DOT -eq 1 ] ; then
	opt -dot-cfg $dir/$name.bc -o $dir/$name.bc
fi


echo "Generating SMT formula..."
gensmtformula
echo "Adding counter to SMT formula..."

if [ $DOMATCHING -eq 1 ] ; then
	TEXFILE=benchmarksedges.tex
	MATCHING=".edges"
    matchingoption=" --matchingfile $MATCHINGFILE --smtmatching $MATCHINGFILE.smtmatch "
else
	TEXFILE=benchmarks.tex
	MATCHING=""
fi
if [ $PRINTSYNTACTIC -eq 1 ] ; then
    printsyntacticoption=" --printlongestsyntactic $LONGESTSYNTACTICFILE "
fi

if [ $RECURSIVECUTS -eq 1 ] ; then
	printcutsoption=" --recursivecuts --printcutslist  $dir/$name.cuts"
fi

# create the formula with summaries	
python $SCRIPTDIR/generateSMTwcet.py $dir/$name.gen $matchingoption $printsyntacticoption $printcutsoption > $dir/${name}${MATCHING}.smt2
FORMULA=$dir/${name}${MATCHING}.smt2

if [ $SUMMARIES -eq 0 ] ; then
	# also create the formula without summaries
	echo -e $RED "SMT formula without summaries" $NORMAL
	python $SCRIPTDIR/generateSMTwcet.py $dir/$name.gen --nosummaries $matchingoption $printsyntacticoption $printcutsoption > $dir/${name}${MATCHING}.nosummaries.smt2
	FORMULA=$dir/${name}${MATCHING}.nosummaries.smt2
fi

echo -e "SMT formula created :" $GREEN "$FORMULA" $NORMAL

# the last line of the .smt2 file gives the longest syntactic path
MAX_BOUND=`tail -n 1 $dir/${name}${MATCHING}.smt2 | rev | cut -d' '  -f1 | rev`
echo -e "max bound = " $RED $MAX_BOUND $NORMAL

echo "Searching for the optimal cost..."
ulimit -t $ULIMIT
if [ $RECURSIVECUTS -eq 1 ] ; then
	cutsfile="$dir/$name.cuts"
	smtopt=$SCRIPTDIR/smtopt/smtopt2
else
	smtopt=$SCRIPTDIR/smtopt/smtopt
fi

echo $SCRIPTDIR/$smtopt $FORMULA cost $cutsfile -v $PRINTMODEL -M $MAX_BOUND 
$SCRIPTDIR/$smtopt $FORMULA cost $cutsfile -v $PRINTMODEL -M $MAX_BOUND 2> $FORMULA.time.smtopt\
>$FORMULA.smtopt 
HASTERMINATED=$?
echo "process terminated with code " $HASTERMINATED

function processresult {
	llvm-dis $dir/$name.bc
	LLVMSIZE=`cat $dir/$name.ll | wc -l`
	NBLLVMBLOCKS=`cat $FORMULA | grep "declare-fun b_" | wc -l`
	NBCUTS=`tail -n 2  $FORMULA | grep "NB_CUTS"  | cut -d ' ' -f 3 `
	case $HASTERMINATED in
		0)
			SMTRES=`tail -n 2  $FORMULA.smtopt | grep "maximum value"  | cut -d ' ' -f 7 `
			SMTTIME=`tail -n 2  $FORMULA.smtopt | grep "Computation time is"  | cut -d ' ' -f 4 `
			# arrondi:
			SMTTIME=`awk "BEGIN { printf \"%.1f\n\", $SMTTIME }"`
			PERCENTAGE_GAINED=` echo "scale=1; (($MAX_BOUND - $SMTRES)*100/$MAX_BOUND)" | bc`
			;;
		*)
			SMTTIME='$+\\infty$'
			;;
	esac
	# line for the latex tabular
	#echo -e $RED "RESULT:" $NORMAL
	echo -e $RED "RESULT:" $GREEN
	echo -e "Synt. \t& SMT \t& % \t& Time \t\t& Name"
	echo -e \
		$MAX_BOUND "\t&"\
		$SMTRES "\t&"\
		$PERCENTAGE_GAINED "\t&"\
		$SMTTIME "\t& "\
		$LLVMSIZE "\t& "\
		$NBLLVMBLOCKS "\t& "\
		$name $NORMAL
	
	# add a \ before any occurence of _ in the name, so that LaTeX does not
	# complain...
	#name=`echo $name | sed  's/_/\\\\_/g'`
	
	NAMEEXISTS=`grep "$name " benchmarkslist.tex`
	if [ "$NAMEEXISTS" = "" ] ; then
		echo -e \
			$name "\t& "\
			$LLVMSIZE "\t& "\
			$NBLLVMBLOCKS ' \\\\ '\
			>> benchmarkslist.tex
	fi 
	
	NAMEEXISTS=`grep "$name " $TEXFILE`
	if [ $SUMMARIES -eq 0 ] ; then
		if [ "$NAMEEXISTS" = "" ] ; then
			echo -e \
			$name "\t& "\
			$MAX_BOUND "\t&"\
			$SMTRES "\t&"\
			${PERCENTAGE_GAINED}\\% "\t&"\
			TIME-"$name " "\t& "\
			$SMTTIME "\t& "\
			$NBCUTS ' \\\\ '\
	    	>> $TEXFILE
		else
			sed -i "s/TIME-$name /$SMTTIME/g" $TEXFILE
		fi
	else
		if [ "$NAMEEXISTS" = "" ] ; then
			echo -e \
			$name "\t& "\
			$MAX_BOUND "\t&"\
			$SMTRES "\t&"\
			${PERCENTAGE_GAINED}\\% "\t&"\
			$SMTTIME "\t& "\
			TIME-$name "\t& "\
			$NBCUTS ' \\\\ '\
	    	>> $TEXFILE
		else
			sed -i "s/TIME-$name /$SMTTIME/g" $TEXFILE
		fi
	fi
}

processresult
