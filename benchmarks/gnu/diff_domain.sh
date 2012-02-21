#!/bin/bash

LATEX=1

TECHNIQUE=$1
D1=$2
D2=$3

function diff_domains {
	for k in `seq 0 3` ; do 
		RES[$k]=0
	done
	for i in `find ./results_domain -name "*\\$$1\\$$2\\$$3.res"` ; do
		if [ ! -z `tail -n 2 $i | grep MATRIX:`  ] ; then
			k=0
			for j in `tail -n 1 $i` ; do
				RES[$k]=$[${RES[$k]}+$j]
				k=$(($k+1))
			done
		fi
	done
	TOTAL=$[${RES[0]}+${RES[1]}+${RES[2]}+${RES[3]}]
	for k in `seq 0 3` ; do 
		AVG[$k]=`echo "scale=2;${RES[$k]}*100/$TOTAL"| bc`
	done
	echo -n ${AVG[1]} \& ${AVG[2]} \& ${AVG[3]}
}

function diff_project {
	if [ -d results_domain ] ; then	
		echo -n "$1 & "
		#diff_domains $TECHNIQUE $D1 $D2
		#echo -n " \& "
		diff_domains lw+pf pk oct
		echo -n " & "
		diff_domains lw+pf pk box
		echo -n " & "
		diff_domains lw+pf oct box
		echo -n " & "
		diff_domains lw+pf pk pkeq
		#echo -n " & "
		#diff_domains lw+pf oct pkeq
		echo -n " & "
		diff_domains lw+pf pk ppl_grid
		echo -n " & "
		diff_domains lw+pf ppl_poly_bagnara ppl_poly
		echo " \\\\ \\hline"
	fi
}

for dir in `ls` ; do
	if [ -d $dir ] ; then
		cd $dir
		diff_project $dir
		cd ..
	fi
done



