#!/bin/bash

search_dir=$1
extension=$2
output_dir=$3

if [ ! -d $output_dir ] ; then
	mkdir $output_dir
fi

for i in `find $search_dir -name "*.$extension"` ; do
	filename=`basename $i`
	cp $i $output_dir/$filename.bc
done
