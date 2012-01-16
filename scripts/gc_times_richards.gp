#
# version 1
#
set style data histogram
set style histogram rowstacked
set style fill solid
set boxwidth 0.5
set key invert samplen 0.2
set key samplen 0.2
set bmargin 3
set offset 0,2,0,0

set title "Clusters of stacked histograms"

plot newhistogram "Machine A" lt 1, \
     'stack+cluster.dat' index 0 u 2:xtic(1) title "col 2", \
     '' index 0 u 3 title "col 3", \
     '' index 0 u 4 title "col 4", \
     '' index 0 u 5 title "col 5", \
     '' index 0 u 6 title "col 6", \
     newhistogram "Machine B" lt 1, \
     'stack+cluster.dat' index 1 u 2:xtic(1) notitle, \
     '' index 1 u 3 notitle, \
     '' index 1 u 4 notitle, \
     '' index 1 u 5 notitle, \
     '' index 1 u 6 notitle
