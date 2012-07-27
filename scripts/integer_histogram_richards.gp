set term postscript eps enhanced monochrome
set size 0.7,0.7
set output "integer_histogram_gcbench.eps"
set title "Integer histogram (GCBench Benchmark)"
set datafile separator "," #csv is comma separated
set yrange [0:]      #plot starting from 0
set ylabel "Number of Integers created"
set logscale x
set logscale y
set yrange [1:]
plot "benchmark_results/GCBench_integer_histogram.csv" using 1:2 ti "positive" with points
