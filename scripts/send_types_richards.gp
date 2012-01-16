set terminal postscript eps enhanced solid color
set output "send_types_richards.eps"
set title "percentage of primitive sends (Richards Benchmark)"
set xtics rotate by -90    #rotate labels
set datafile separator "," #csv is comma separated
set style fill solid 1.00 border 0 #fill bars
set boxwidth 0.6
set ylabel "percentage"
#set logscale y
plot "benchmark_results/send_types_richards.csv" using 2:xtic(1) ti "sends implemented by primitives" with boxes
