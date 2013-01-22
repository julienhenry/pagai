#!/bin/bash

function usage () {
echo "
Usage:
./process_results.sh <list of files>
"
}

function getFileBetween () {
	cat $3 | awk '	BEGIN {aff=0;aff2=0} {
				if (aff2==1) {
					aff=1; aff2=0
				}
				if ($0 ~ /'$1'/) {
					aff2=1
				} else {
					if ($0 ~ /'$2'/) {
						aff=0
					} 
				}; 
				if (aff==1) {
					print $0
				};
			} 
		'
}

echo $*

SECONDS=0
MSECONDS=0

getFileBetween "TIME:" "TIME_END" res/verisec_sendmail__tTflag_arr_one_loop_unsafe.res.incr > /tmp/.tmp_process_result
while read s ms line
do
	SECONDS[$line]=$((${SECONDS[$line]}+$s))
	MSECONDS[$line]=$((${MSECONDS[$line]}+$ms))
done < /tmp/.tmp_process_result

echo $MSECONDS
echo $SECONDS
echo $MSECONDS


exit 0
