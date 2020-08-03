#! /usr/bin/gnuplot
#
# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
# 	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
# 	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
# 	lab2b_3.png ... successful iterations vs. threads for each synchronization method. 
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists. 
# 	lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# throughput vs. number of threads for mutex and spin-lock synchronized list operations.
set title "Throughput vs Number of threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only single threaded, un-protected, non-yield results
plot \
	 "< grep 'list-none-m,[0-9]*,1000,1,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title 'w Mutex' with linespoints lc rgb 'red', \
	 "< grep 'list-none-s,[0-9]*,1000,1,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title 'w Spin-Lock' with linespoints lc rgb 'green'


set title "Mean time per mutex wait and Mean time per operation for mutex operations"
set xlabel "Threads"
set logscale x 2
set xrange [1:32]
set ylabel "Time"
set logscale y 10
set output 'lab2b_2.png'
# note that unsuccessful runs should have produced no output
plot \
	 "< grep 'list-none-m,[0-9]*,1000,1,' lab2_list.csv" using ($2):($8) \
	title 'wait-for-lock time' with points lc rgb 'red', \
	 "< grep 'list-none-m,[0-9]*,1000,1,' lab2_list.csv" using ($2):($7) \
	title 'average time per operation' with points lc rgb 'green'
	 
set title "Successful Iterations vs. Threads for each Synchronization Method"
set xlabel "Threads"
set logscale x 2
set xrange [0:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
# note that unsuccessful runs should have produced no output
plot \
	"< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2_list.csv" using ($2):($3) \
	title 'no synchronization' with points lc rgb 'red', \
	"< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2_list.csv" using ($2):($3) \
	title 'mutex protected' with points lc rgb 'green', \
	"< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2_list.csv" using ($2):($3) \
	title 'spinlock protected' with points lc rgb 'blue'
#
# "no valid points" is possible if even a single iteration can't run
#

set title "Throughput vs. Number of Threads for mutex synchronized partitioned lists"
set xlabel "Threads"
set logscale x 2
set xrange [0:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'
# note that unsuccessful runs should have produced no output
plot \
	 "< grep 'list-none-m,[0-9]*,1000,1,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '1-list' with points lc rgb 'red', \
	 "< grep 'list-none-m,[0-9]*,1000,4,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '4-lists' with points lc rgb 'green', \
	 "< grep 'list-none-m,[0-9]*,1000,8,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '8-lists' with points lc rgb 'blue', \
	 "< grep 'list-none-m,[0-9]*,1000,16,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '16-lists' with points lc rgb 'violet'


set title "Throughput vs. Number of Threads for spin-lock-synchronized partitioned lists"
set xlabel "Threads"
set logscale x 2
set xrange [0:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'
# note that unsuccessful runs should have produced no output
plot \
	 "< grep 'list-none-s,[0-9]*,1000,1,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '1-list' with points lc rgb 'red', \
	 "< grep 'list-none-s,[0-9]*,1000,4,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '4-lists' with points lc rgb 'green', \
	 "< grep 'list-none-s,[0-9]*,1000,8,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '8-lists' with points lc rgb 'blue', \
	 "< grep 'list-none-s,[0-9]*,1000,16,' lab2_list.csv" using ($2):(1000000000/($7)) \
	title '16-lists' with points lc rgb 'violet'
	