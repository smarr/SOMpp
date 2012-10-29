reset
fontsize=12
set terminal postscript enhanced eps fontsize color
set output "richards_benchmark_additional_allocation.eps"
set title "Richards Benchmark"
set datafile separator "," #csv is comma separated
set yrange [0:]            #plot starting from 0
set ylabel "Average execution time (ms)"
set style data histograms  #plot histogram style
set style histogram errorbars gap 2 lw 1
set style fill solid noborder
set grid ytics
set key left
plot "benchmark_results/generational_nocache_noTagging_richards.csv" using 2:3:xtic(1) ti "Richards generational", \
     "benchmark_results/generational_additional_allocation_richards.csv" using 2:3 ti "Richards generational (additional allocation)", \
     "benchmark_results/copying_nocache_noTagging_richards.csv" using 2:3 ti "Richards copying", \
     "benchmark_results/copying_additional_allocation_richards.csv" using 2:3 ti "Richards copying (additional allocation)", \
     "benchmark_results/mark_sweep_nocache_noTagging_richards.csv" using 2:3 ti "Richards mark-sweep", \
     "benchmark_results/mark_sweep_additional_allocation_richards.csv" using 2:3 ti "Richards mark-sweep (additional allocation)"
