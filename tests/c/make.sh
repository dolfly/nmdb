#!/bin/bash

set -e

USAGE="\
Use: $0 [build|debug_build|strict_build|profile_build|clean]
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
	"help" | "--help" | "-h" | "" | *)
		echo $USAGE
		exit 1
		;;
esac;


for p in TIPC TCP UDP SCTP MULT; do
	for v in NORMAL CACHE SYNC; do
		OP=`echo $p-$v | tr '[A-Z]' '[a-z]'`
		TF="-DUSE_$p=1 -DUSE_$v=1"

		echo " * $OP:"
		for t in 1 2 3 "set" "get" "del" "incr"; do
			echo "   * $t"
			if [ "$CLEAN" == 1 ]; then
				rm -f $t-$OP
			else
				# build only if src is newer than the binary
				if [ "$t.c" -nt "$t-$OP" ]; then
					cc $t.c -lnmdb $ALLCF $TF -o $t-$OP
				fi
			fi
		done
	done
done

