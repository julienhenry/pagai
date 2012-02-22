#!/bin/bash

for k in `seq 0 3` ; do 
	ALL[$k]=0
done

LATEX=0
PRINT_TIME=0

for dir in `ls` ; do
	if [ -d $dir ] ; then
		cd $dir
		for k in `seq 0 3` ; do 
			RES[$k]=0
		done
		for k in `seq 0 3` ; do 
			TIME[$k]=0
		done
		FUNCTIONS=0
		IGNORED=0
		if [ -d res ] ; then	
			for i in `ls res/*.res.narrow` ; do
				basename=`basename $i`
				basename=${basename%%.*}
			
				if [ ! -z `tail -n 2 $i | grep MATRIX:`  ] ; then
					k=0
					for j in `tail -n 1 $i` ; do
						RES[$k]=$[${RES[$k]}+$j]
						k=$(($k+1))
					done
				fi

				if [ ! -z `tail -n 21 $i | grep FUNCTIONS:`  ] ; then
					NFUNC=`tail -n 20 $i | head -n 1`
					FUNCTIONS=$[$FUNCTIONS+$NFUNC]
				fi

				if [ ! -z `tail -n 18 $i | grep IGNORED:`  ] ; then
					NFUNC=`tail -n 17 $i | head -n 1`
					IGNORED=$[$IGNORED+$NFUNC]
				fi

				if [ ! -z `tail -n 15 $i | grep TIME:`  ] ; then
					k=0
					for j in `tail -n 14 $i | head -n 5` ; do
						TIME[$k]=$[${TIME[$k]}+$j]
						k=$(($k+1))
					done
				fi
			
			done
			TOTAL=$[${RES[0]}+${RES[1]}+${RES[2]}+${RES[3]}]
			for k in `seq 0 27` ; do 
			AVG[$k]=`echo "scale=2;${RES[$k]}*100/$TOTAL"| bc`
			done
			TIME_1=`echo "scale=0;(${TIME[0]}*1000000+${TIME[1]})/1000000" | bc`
			TIME_2=`echo "scale=0;(${TIME[2]}*1000000+${TIME[3]})/1000000" | bc`
			if [ $LATEX -eq 0 ] ; then
				echo "#####"
				echo $dir
				echo "IGNORED : $IGNORED / $FUNCTIONS"
				echo ""
				echo TIME
				echo $TIME_S S
				echo $TIME_LW LW
				echo $TIME_PF PF
				echo $TIME_C LW+PF
				echo $TIME_DIS DIS
				echo ""
				echo $TOTAL
				echo EQ LT GT UN
				echo ${RES[0]} ${RES[1]} ${RES[2]} ${RES[3]}		
				echo ""
				echo EQ LT GT UN
				echo ${AVG[0]} ${AVG[1]} ${AVG[2]} ${AVG[3]}
				echo "#####"
			fi
			for k in `seq 0 3` ; do 
			ALL[$k]=$[${RES[$k]}+${ALL[$k]}]
			done
		fi
		cd ..
	fi
done


echo "#####"
echo "TOTAL"
echo EQ LT GT UN
echo ${ALL[0]} ${ALL[1]} ${ALL[2]} ${ALL[3]}
echo "#####"

TOTAL=$[${ALL[0]}+${ALL[1]}+${ALL[2]}+${ALL[3]}]
for k in `seq 0 27` ; do 
ALL[$k]=`echo "scale=2;${ALL[$k]}*100/$TOTAL"| bc`
done

echo "#####"
echo "TOTAL"
echo EQ LT GT UN
echo G/S      ${ALL[0]} ${ALL[1]} ${ALL[2]} ${ALL[3]}			
echo "#####"
