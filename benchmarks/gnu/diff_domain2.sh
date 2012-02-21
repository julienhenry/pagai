#!/bin/bash

LATEX=0

for k in `seq 0 3` ; do 
	TOT[$k]=0
done

for dir in `ls` ; do
	if [ -d $dir ] ; then
		cd $dir
		for k in `seq 0 3` ; do 
			RES[$k]=0
		done
		if [ -d results_domain ] ; then	
			for i in `find ./results_domain -name "*\\$$1\\$$2\\$$3.res"` ; do
				if [ ! -z `tail -n 2 $i | grep MATRIX:`  ] ; then
					k=0
					for j in `tail -n 1 $i` ; do
						RES[$k]=$[${RES[$k]}+$j]
						k=$(($k+1))
					done
				fi
			done
			for k in `seq 0 3` ; do 
				TOT[$k]=$[${TOT[$k]}+${RES[$k]}]
			done
			TOTAL=$[${RES[0]}+${RES[1]}+${RES[2]}+${RES[3]}]
			for k in `seq 0 3` ; do 
				AVG[$k]=`echo "scale=2;${RES[$k]}*100/$TOTAL"| bc`
			done
			if [ $LATEX -eq 1 ] ; then
				echo $dir $1 $2 $3 ${AVG[0]} ${AVG[1]} ${AVG[2]} ${AVG[3]}
			else
				echo "#####"
				echo $TOTAL
				echo $dir
				echo $1 $2 $3
				echo ${RES[0]} ${RES[1]} ${RES[2]} ${RES[3]}
				echo "#####"
			fi
		fi
		cd ..
	fi
done

TOTAL=$[${TOT[0]}+${TOT[1]}+${TOT[2]}+${TOT[3]}]
for k in `seq 0 3` ; do 
	AVG[$k]=`echo "scale=2;${TOT[$k]}*100/$TOTAL"| bc`
done
echo TOTAL
echo "#####"
echo $TOTAL
echo $1 $2 $3
echo ${AVG[0]} ${AVG[1]} ${AVG[2]} ${AVG[3]}
echo "#####"


