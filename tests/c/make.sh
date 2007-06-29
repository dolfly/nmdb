#!/bin/bash

set -e

USAGE="\
Use: build.sh [build|debug_build|strict_build|profile_build|clean]
"


ALLCF="-D_XOPEN_SOURCE=500 -fPIC -std=c99 -Wall -O3"

case "$1" in
	"build" )
		# defaults are just fine for build
		;;
	"debug_build" )
		ALLCF="$ALLCF -g"
		;;
	"strict_build" )
		ALLCF="$ALLCF -ansi -pedantic"
		;;
	"profile_build" )
		ALLCF="$ALLCF -g -pg -fprofile-arcs -ftest-coverage"
		;;
	"clean" )
		CLEAN=1
		;;
	"help" | "--help" | "-h" | "" )
		echo $USAGE
		exit 1
		;;
esac;


for p in TIPC TCP UDP MULT; do
	for v in NORMAL CACHE SYNC; do
		OP=`echo $p-$v | tr '[A-Z]' '[a-z]'`
		TF="-DUSE_$p=1 -DUSE_$v=1"

		echo " * $OP:"
		for t in 1 2 3 "set" "get" "del"; do
			echo "   * $t"
			if [ "$CLEAN" == 1 ]; then
				rm -f $t-$OP
			else
				cc -lnmdb $ALLCF $TF -o $t-$OP $t.c
			fi
		done
	done
done

