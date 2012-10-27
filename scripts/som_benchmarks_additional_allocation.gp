reset
fontsize=12
set terminal postscript enhanced eps fontsize color
set output "som_benchmarks_additional_allocation.eps"
set title "SOM Benchmarks"
set datafile separator "," #csv is comma separated
set yrange [0:]            #plot starting from 0
set xtics rotate by -45    #rotate labels
set ylabel "Average execution time (ms)"
set style data histograms  #plot histogram style
set style histogram errorbars gap 2 lw 1
set style fill solid noborder
set grid ytics
plot "benchmark_results/generational_nocache_noTagging_som.csv" using 2:3:xtic(1) ti "SOM++ generational", \
     "benchmark_results/generational_additional_allocation_som.csv" using 2:3 ti "SOM++ generational (additional allocation)", \
     "benchmark_results/copying_nocache_noTagging_som.csv" using 2:3 ti "SOM++ copying", \
     "benchmark_results/copying_additional_allocation_som.csv" using 2:3 ti "SOM++ copying (additional allocation)", \
     "benchmark_results/mark_sweep_nocache_noTagging_som.csv" using 2:3 ti "SOM++ mark-sweep", \
     "benchmark_results/mark_sweep_additional_allocation_som.csv" using 2:3 ti "SOM++ mark-sweep (additional allocation)"
