#!/bin/bash

# Automated nmdb performance test
# Should be ran from somewhere inside the git repository

COUNT=2		# How many times we should repeat the test
TIMES=5000	# Times parameter for the tests


set -e

# set up the temporary directory, and the cleanup routine
TMP=`mktemp -d -t nmdb-perf-test.XXXXXXXXXX` || \
	( echo "error: Can't create tmp directory"; exit 1 )
function atexit() {
	server_stop
	rm -r "$TMP"
}
trap atexit EXIT


NMDBPID=
function server_start() {
	# launch the server
	techo "start nmdb"
	rm -f $TMP/database $OUTDIR/nmdb.log
	./nmdb/nmdb -f -d $TMP/database -o $OUTDIR/nmdb.log &
	NMDBPID=$!

	# wait a bit so the clients don't try to talk too early
	sleep 0.5
}

function server_stop() {
	if [ "$NMDBPID" != "" ]; then
		techo "stop nmdb"
		kill $NMDBPID || true
		wait $NMDBPID
	fi
	NMDBPID=
}



function techo() {
	echo `date "+%H:%M:%S.%N"` "$@"
}

# helper function used to run a single test that takes the same parameters:
# times, ksize, vsize; must be called from inside test_current
function run_test() {
	t1=$1
	lcount=$2
	ltimes=$3

	for t2 in mult tipc tcp udp sctp; do
		for t3 in cache; do
			t="$t1-$t2-$t3"
			techo "   " $t $@
			rm -f $OUTDIR/$t.out
			for i in `seq 1 $lcount`; do
				# restart server
				server_start
				echo -n "      "
				for s in 4 32 128 512 1024 \
						`seq 2048 2048 30000`; do
					echo $s `tests/c/$t $ltimes $s $s` \
							>> $OUTDIR/$t.out
					sleep 0.3
					echo -n " $s"
				done
				# gnuplot splits the data sets with this empty
				# line, otherwise we get weird graphics when
				# $COUNT > 2
				echo >> $OUTDIR/$t.out
				echo
				server_stop
			done
		done
	done
}


# tests the current revision
function test_current() {
	# go to the repo root
	cd "`git rev-parse --show-cdup`"

	# create the output directory
	OUTDIR="tests/perf/out/`git describe --always`$SUFFIX"
	techo mkdir $OUTDIR
	mkdir -p $OUTDIR

	# record the environment
	(
		echo date `date --rfc-3339=seconds`
		echo testing `git describe --always`
		#techo using perf ${OLDCOMMIT:-`git describe --always`}
		echo
		uname -a
		gcc --version | grep gcc
		echo
		cat /proc/cpuinfo | grep 'model name'
		cat /proc/meminfo | grep MemTotal
	) > $OUTDIR/environment

	# build
	techo build
	#make BACKEND=bdb > $OUTDIR/make.out
	# no porque asumimos en el sistema
	#make > $OUTDIR/make.out
	cd tests/c
	./make.sh build > $TMP/tests-make.out
	cd - > /dev/null

	# record the vmstat output while the test runs
	techo vmstat
	vmstat 1 > $OUTDIR/vmstat.out &
	VMSTATPID=$!

	# wait a bit for the server to come up
	sleep 1

	# run the tests
	techo tests

	for t1 in 2 3; do
		run_test $t1 $COUNT $TIMES
	done

	# kill everything; the "|| true" is needed to prevent the script from
	# dying if the process are already dead
	techo "kill vmstat ($VMSTATPID)"
	kill -9 $VMSTATPID || true

	# wait for them to die
	wait $NMDBPID $VMSTATPID > /dev/null 2> /dev/null || true

	techo done
}



function write_current_graph_scripts() {
	SUFFIX="$1"
	V="`git describe --always`$SUFFIX"

	# go to the results dir
	cd "`git rev-parse --show-cdup`"

	mkdir -p tests/perf/graph
	DST=`pwd`/tests/perf/graph

	gnuplot <<EOF

set terminal png size 1280,1024 enhanced

set output "$DST/$V-2-cache.png"
set title "$V - test 2, tipc"
set xlabel "key+value size"
set ylabel "ops/seg"
plot \
	'tests/perf/out/$V/2-tipc-cache.out' using \
		(\$1 * 2):($TIMES / (\$2 / 1000000.0)) title "cache: get" \
		with linespoints, \
	'tests/perf/out/$V/2-tipc-cache.out' using \
		(\$1 * 2):($TIMES / (\$3 / 1000000.0)) title "cache: set" \
		with linespoints, \
	'tests/perf/out/$V/2-tipc-cache.out' using \
		(\$1 * 2):($TIMES / (\$4 / 1000000.0)) title "cache: del" \
		with linespoints \

set output "$DST/$V-3-cache.png"
set title "$V - test 3, tipc"
set xlabel "key/value size"
set ylabel "(get+set+del)/seg"
plot \
	'tests/perf/out/$V/3-tipc-cache.out' using \
		1:($TIMES / (\$2 / 1000000.0)) title "cache"

EOF
	cd - > /dev/null

}


if [ "$2" == "" ]; then
	SUFFIX=""
else
	SUFFIX="-$2"
fi

case $1 in
    ""|"help"|"-h"|"--help")
	echo "Usage: perf.sh test|graph [suffix]"
	echo
	echo "Run from somewhere inside the git repository; output will"
	echo "be put in tests/pref/out/<git commit>[-suffix]."
	exit 1
	;;
    "test")
	test_current $SUFFIX
	;;
    "graph")
	write_current_graph_scripts $SUFFIX
	;;
    *)
	echo "Unknown operation"
	exit 1
esac

