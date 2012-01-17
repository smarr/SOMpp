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

plot newhistogram "generational+cache" lt 1, \
       'benchmark_results/generational_cache_noTagging_richards.csv' using 2:xtic(1) title "interpretation time", \
       '' using 4 title "gc time", \
     newhistogram "generational+cache+tagging" lt 1, \
       'benchmark_results/generational_cache_tagging_richards.csv' using 2:xtic(1), \
       '' using 4
