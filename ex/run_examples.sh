#!/bin/bash

for FILENAME in `ls *.{c,cpp}` ; do
	echo "Running $FILENAME..."
	./analyzer.sh -i $FILENAME -u  > results/$FILENAME.result
done
