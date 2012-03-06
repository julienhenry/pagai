#!/bin/bash
if [ -z $1 ] ; then
	echo error : please use the command \"make ndiff\"
	exit
fi

DIR=`pwd`
cd $1


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
			ITERATIONS[$k]=0
		done
		for k in `seq 0 7` ; do 
			TIME[$k]=0
		done
		FUNCTIONS=0
		IGNORED=0
		if [ -d res ] ; then	
			for i in `ls res/*.res.narrow` ; do
				basename=`basename $i`
				basename=${basename%%.*}
			
				echo $i
				if [ ! -z `tail -n 2 $i | grep MATRIX:`  ] ; then
					k=0
					for j in `tail -n 1 $i` ; do
						RES[$k]=$[${RES[$k]}+$j]
						k=$(($k+1))
					done
				fi


				if [ ! -z `tail -n 13 $i | grep ITERATIONS`  ] ; then
					k=0
					for j in `tail -n 12 $i | head -n 2` ; do
						ITERATIONS[$k]=$[${ITERATIONS[$k]}+$j]
						k=$(($k+1))
					done
				fi

				if [ ! -z `tail -n 19 $i | grep TIME`  ] ; then
					k=0
					for j in `tail -n 18 $i | head -n 4` ; do
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
			TIME_3=`echo "scale=0;(${TIME[4]}*1000000+${TIME[5]})/1000000" | bc`
			TIME_4=`echo "scale=0;(${TIME[6]}*1000000+${TIME[7]})/1000000" | bc`
			for k in `seq 0 3` ; do 
				ALL[$k]=$[${RES[$k]}+${ALL[$k]}]
			done
			echo "             ASC      DESC"
			echo "IMPROVED ${ITERATIONS[0]} ${ITERATIONS[1]}"
			echo "CLASSIC  ${ITERATIONS[2]} ${ITERATIONS[3]}"
			echo ""
			echo "TIME"
			echo "IMPROVED" $TIME_1 
			echo "CLASSIC " $TIME_2 
			echo ""
			echo "SAME RESULT:"
			echo "IMPROVED " $TIME_3 
			echo "CLASSIC " $TIME_4 
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

cd $DIR
