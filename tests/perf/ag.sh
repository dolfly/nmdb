#!/bin/bash

# Automated nmdb performance test - aggregated graphics
# Should be ran from somewhere inside the git repository

TIMES=5000	# Times parameter used for the tests

set -e


# aggregate the results prepending the version, but we need a table because
# gnuplot doesn't like strings as values
cd `git rev-parse --show-cdup`
cd tests/perf/out
mkdir -p ../ag-data
rm -f ../ag-data/*

declare -a COMMITS
N=0

for i in *; do
	COMMITS[$N]=$i
	for f in 2-tipc-cache.out 3-tipc-cache.out; do
		while read L; do
			if [ "$L" != "" ]; then
				echo $N $L >> ../ag-data/$f
			else
				# preserve data set delimiters for gnuplot
				echo >> ../ag-data/$f
			fi
		done < $i/$f
	done
	N=$[ $N + 1 ]
done


# write the gnuplot script
cd `git rev-parse --show-cdup`
cd tests/perf
mkdir -p graph
DST=`pwd`/graph

# 2-tipc-cache (3d)
cat > graph/ag-tmp.sh <<EOF
#!/usr/bin/env gnuplot

set terminal png size 1280,1024 enhanced

set title "all versions, test 2, tipc"
set ticslevel 0

set output "$DST/3D-2-cache.png"
set xtics axis offset -8 ( \\
EOF

N=0
for c in ${COMMITS[@]}; do
	echo "\"$c\" $N, \\" >> graph/ag-tmp.sh
	N=$[ $N + 1 ]
done

cat >> graph/ag-tmp.sh <<EOF
	"" $N)
set xlabel "commit" offset 3
set ylabel "key/value size"
set zlabel "ops/seg"
splot \
	'ag-data/2-tipc-cache.out' using \
		1:(\$2 * 2):($TIMES / (\$3 / 1000000.0)) \
		with linespoints title "cache get"
EOF

# 2-tipc-cache (2d)
cat >> graph/ag-tmp.sh <<EOF
set output "$DST/2D-2-cache.png"
set title "all versions, test 2, tipc"
unset xtics
set xtics autofreq
set xlabel "key/value size"
set ylabel "ops/seg"
plot \\
EOF

for c in ${COMMITS[@]}; do
cat >> graph/ag-tmp.sh <<EOF
	'out/$c/2-tipc-cache.out' \\
		using (\$1 * 2):($TIMES / (\$2 / 1000000.0)) \\
		with linespoints title "cache get $c", \\
EOF
done

cat >> graph/ag-tmp.sh <<EOF
	0 notitle
EOF

# 3-tipc-cache (2d)
cat >> graph/ag-tmp.sh <<EOF
set output "$DST/2D-3-cache.png"
set title "all versions, test 3, tipc"
unset xtics
set xtics autofreq
set xlabel "key+value size"
set ylabel "(get+set+del)/seg"
plot \\
EOF

for c in ${COMMITS[@]}; do
cat >> graph/ag-tmp.sh <<EOF
	'out/$c/3-tipc-cache.out' \\
		using (\$1 * 2):($TIMES / (\$2 / 1000000.0)) \\
		with linespoints title "cache $c", \\
EOF
done

cat >> graph/ag-tmp.sh <<EOF
	0 notitle
EOF

chmod +x graph/ag-tmp.sh

./graph/ag-tmp.sh
rm ./graph/ag-tmp.sh

cd - > /dev/null


