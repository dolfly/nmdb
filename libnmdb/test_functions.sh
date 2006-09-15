#!/bin/bash

# Useful functions for performing automated performance testing.


# run test2[c|d] varying KSIZE and VSIZE (which are equal).
# $1 == "c" or "d" depending on which test2 to run
# $2 == TIMES to pass to test2
function var_sizes {
	if [ "$1" == "" ] && [ "$2" == ""]; then
		return;
	fi;

	for i in 8 16 64 128 512 1024 2048 5120 8192 10240 20480 32768; do
		for j in 1 2 3; do
			echo $i `./test2$1 $2 $i $i`;
		done;
	done;
}


# run test2[c|d] varying TIMES.
# $1 == "c" or "d" depending on which test2 to run
# $2 == KSIZE and VSIZE to pass to test2
function var_times {
	if [ "$1" == "" ] && [ "$2" == ""]; then
		return;
	fi;

	for i in `seq 500 500 30000`; do
		for j in 1 2 3; do
			echo $i `./test2$1 $i $2 $2`;
		done;
	done;
}


# run test2[c|d] and return the amount of operations per second
# $1 == "c" or "d" depending on which test2 to run
# $2 == list of TIMES to use (eg. "1000 2000 5000")
# $3 == KSIZE and VSIZE
function ops_per_sec {
	if [ "$1" == "" ] && [ "$2" == ""] && [ "$3" == ""]; then
		return;
	fi;

	for i in $2; do
		for j in 1 2 3; do
			OUT=`./test2$1 $i $3 $3`;
			ST=`echo $OUT | cut -d ' ' -f 1`;
			GT=`echo $OUT | cut -d ' ' -f 2`;
			DT=`echo $OUT | cut -d ' ' -f 3`;
			MS=`echo $OUT | cut -d ' ' -f 4`;

			SO=$[ $ST / $i ];
			GO=$[ $GT / $i ];
			DO=$[ $DT / $i ];

			echo $i $SO $GO $DO $MS;
		done;
	done;
}


#
# sample invocations
#
# srv:X means the server is running with -c X

# var_sizes c 5000 > var_sizes_c_5000_srv:1
# var_sizes c 5000 > var_sizes_c_5000_srv:128
# ops_per_sec c "`seq 2000 1000 10000`" 32 > ops_per_sec_c_32_srv:1
# ops_per_sec c "`seq 2000 1000 10000`" 32 > ops_per_sec_c_32_srv:128
# ops_per_sec d "`seq 2000 1000 10000`" 32 > ops_per_sec_d_32_srv:1
# ops_per_sec d "`seq 2000 1000 10000`" 32 > ops_per_sec_d_32_srv:128
# var_times c 32 > var_times_c_32_srv:1
# var_times c 32 > var_times_c_32_srv:16
# var_times c 32 > var_times_c_32_srv:126


