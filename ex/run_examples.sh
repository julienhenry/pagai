#!/bin/bash

OLD_DIR=`pwd`
cd /home/jhenry/m2r/ex

for FILENAME in `ls *.{c,cpp}` ; do
	echo "Running $FILENAME..."
	analyzer.sh -i $FILENAME -u -g > results/$FILENAME.result
done



cd $OLD_DIR
