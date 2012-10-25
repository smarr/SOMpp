reset
fontsize = 12
set terminal postscript enhanced eps fontsize solid color
set output "send_types_gcbench.eps"
set title "percentage of primitive sends (GCBench)"
set xtics rotate by -90    #rotate labels
set datafile separator "," #csv is comma separated
set style fill solid 1.00 noborder
set boxwidth 0.9
set ylabel "number of sends"
plot "benchmark_results/GCBench_send_types.csv" using ($3+$4):xtic(1) ti "primitive calls" with boxes, \
     ""                                                    using 4:xtic(1) ti "non primitive calls" with boxes
