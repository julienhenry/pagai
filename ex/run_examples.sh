#!/bin/bash

for FILENAME in `ls bin` ; do
	echo "Running $FILENAME..."
	../src/pagai -i bin/$FILENAME -c  > results/$FILENAME.result
done
