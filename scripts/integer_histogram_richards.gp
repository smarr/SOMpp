set term postscript eps enhanced solid color
set output "integer_histogram_richards.eps"
set title "Integer histogram (Richards Benchmark)"
set datafile separator "," #csv is comma separated
set yrange [0:]      #plot starting from 0
set ylabel "Number of Integers created"
set logscale x
set logscale y
set yrange [1:]
plot "benchmark_results/RichardsBenchmarks_integer_histogram.csv" using ($1 < 1 ? 1/0 : $1):2 ti "positive" with points, \
     "" using (-$1):2 ti "negative" with points
