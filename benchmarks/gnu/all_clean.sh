#!/bin/bash

if [ -d res ]
then
	rm -rf res
fi


for inode in `ls`
do
if
	[ -d $inode ]
then
	if [ -f $inode/analyze.sh ] ; then
		echo "===================================="
		echo removing results for $inode
		echo
		cd $inode
		rm -rf res
		echo "===================================="
		cd ..
	fi
fi
done
 
