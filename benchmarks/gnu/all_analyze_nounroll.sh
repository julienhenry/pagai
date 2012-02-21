#!/bin/bash

for inode in `ls`
do
if
	[ -d $inode ]
then
	if [ -f $inode/analyze_nounroll.sh ] ; then
		echo "===================================="
		echo $inode
		echo
		cd $inode
		./analyze_nounroll.sh
		echo "End of analysis"
		echo "===================================="
		cd ..
	fi
fi
done
 
