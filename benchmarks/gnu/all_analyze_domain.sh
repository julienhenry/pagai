#!/bin/bash

for inode in `ls`
do
if
	[ -d $inode ]
then
	if [ -f $inode/analyze_domain.sh ] ; then
		echo "===================================="
		echo $inode
		echo
		cd $inode
		./analyze_domain.sh $1 $2 $3
		echo "End of analysis"
		echo "===================================="
		cd ..
	fi
fi
done
 
