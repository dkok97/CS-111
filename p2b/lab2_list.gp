#! /usr/local/cs/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
# 8. avg time for lock per lock operation (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#	lab2b_3.png ... successful iterations vs. threads for each synchronization method.
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists.
# lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.
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

set title "List-1-2B: Throughput vs Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'

plot \
    "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	     title 'ops with mutex' with linespoints lc rgb 'red', \
    "< grep 'list-none-s,[0-9]\\+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
       title 'ops with spin' with linespoints lc rgb 'green',


set title "List-2-2B: Avg wait-for-lock-time and time per operation for mutex"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "avg time/operation(ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
    "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" using ($2):($8) \
	     title 'avg' with linespoints lc rgb 'red', \
    "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" using ($2):($7) \
       title 'ops with spin' with linespoints lc rgb 'green',

set title "List-3-2B: Successful iterations vs. threads for each synchronization method"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
    "< grep 'list-id-none,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3) \
       title 'no_lock' with points lc rgb 'red', \
    "< grep 'list-id-s,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3) \
       title 'spin_lock' with points lc rgb 'blue', \
    "< grep 'list-id-m,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3) \
       title 'mutex_lock' with points lc rgb 'green',


set title "List-4-2B: Throughput vs. number of threads for mutex synchronized partitioned lists."
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
unset xrange
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'

plot \
    "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	     title '1 list' with linespoints lc rgb 'red', \
    "< grep 'list-none-m,[0-9]\\+,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
       title '4 lists' with linespoints lc rgb 'green', \
    "< grep 'list-none-m,[0-9]\\+,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
       title '8 lists' with linespoints lc rgb 'blue', \
    "< grep 'list-none-m,[0-9]\\+,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
       title '16 lists' with linespoints lc rgb 'orange',


set title "List-5-2B: Throughput vs. number of threads for spin-lock-synchronized partitioned lists."
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_5.png'

plot \
    "< grep 'list-none-s,[0-9]\\+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	     title '1 list' with linespoints lc rgb 'red', \
    "< grep 'list-none-s,[0-9]\\+,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
       title '4 lists' with linespoints lc rgb 'green', \
    "< grep 'list-none-s,[0-9]\\+,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
       title '8 lists' with linespoints lc rgb 'blue', \
    "< grep 'list-none-s,[0-9]\\+,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
       title '16 lists' with linespoints lc rgb 'orange',
