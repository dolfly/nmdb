#!/bin/bash

set -e

USAGE="\
Use: build.sh [build|debug_build|strict_build|profile_build|clean]
"


CF="-std=c99 -Wall -O3"
ALLCF="-D_XOPEN_SOURCE=500 -fPIC $CF"

case "$1" in
	"build" )
		# defaults are just fine for build
		;;
	"debug_build" )
		ALLCF="-g $CF"
		;;
	"strict_build" )
		ALLCF="-ansi -pedantic $CF"
		;;
	"profile_build" )
		ALLCF="-g -pg -fprofile-arcs -ftest-coverage $CF"
		;;
	"clean" )
		CLEAN=1
		;;
	"help" | "--help" | "-h" | "" )
		echo $USAGE
		exit 1
		;;
esac;


for p in TIPC TCP MULT; do
	for v in NORMAL CACHE SYNC; do
		OP=`echo $p-$v | tr '[A-Z]' '[a-z]'`
		CF="-DUSE_$p=1 -DUSE_$v=1"

		if [ "$CLEAN" == 1 ]; then
			rm -vf 1-$OP 2-$OP
		else
			echo " * $OP"
			cc -lnmdb $CF -o 1-$OP 1.c
			cc -lnmdb $CF -o 2-$OP 2.c
		fi
	done
done

