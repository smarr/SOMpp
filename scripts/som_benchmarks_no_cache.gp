reset
set term pdf font ",5" 
set output "som_benchmarks.pdf"
set title "SOM Benchmarks"
set datafile separator "," #csv is comma separated
set yrange [0:160+40]            #plot starting from 0
set xtics rotate by -45    #rotate labels
set ylabel "Average execution time (ms)"
set style data histograms  #plot histogram style
set style fill solid 1.00 border 0 #fill bars
set style histogram errorbars gap 2 lw 1
plot 'benchmark_results/som_benchmarks.csv' \
	using 2:3:xtic(1) ti "SOM++ generational", \
	'' using 6:7 ti "SOM++ generational (tagging)", \
	'' using 10:11 ti "SOM++ copying", \
	'' using 14:15 ti "SOM++ copying (tagging)", \
	'' using 18:19 ti "SOM++ mark-sweep", \
	'' using 22:23 ti "SOM++ mark-sweep (tagging)"
