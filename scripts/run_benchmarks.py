#! /usr/bin/python
import os
import sys
from Benchmark import Benchmark

base_path = os.path.abspath(os.path.dirname(sys.argv[0]) + "../")

if __name__ == "__main__":    
    bench = Benchmark(base_path+"/bin/generational_cache_noTagging",
            base_path+"/Examples/Benchmarks/IntegerLoop.som", 10)
    bench.run()
    print "CSV:" + bench.get_csv()


