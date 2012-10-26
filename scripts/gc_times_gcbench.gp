set terminal postscript eps enhanced solid color
set output "gc_times_gcbench.eps"
set style data histogram
set style histogram rowstacked
set style fill solid
set boxwidth 0.5
set key invert samplen 0.2
set key samplen 0.2
set bmargin 3
set offset 0,2,0,0
unset xtics

set title "Clusters of stacked histograms"

plot 'benchmark_results/gcbench_benchmark.csv' using ($2-$4):xtic(1) title "interpretation time", \
       '' using 4 title "gc time"
