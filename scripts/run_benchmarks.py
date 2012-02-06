#! /usr/bin/python
import os
import sys
from os import listdir
from os import rename

from Benchmark import Benchmark
from BenchmarkRunner import BenchmarkRunner
import os.path

base_path = os.path.abspath(os.path.dirname(sys.argv[0]) + "/../")

ITERATIONS = 20

SOM_BENCHMARKS = [
"Bounce.som", "BubbleSort.som", "Dispatch.som", "Fibonacci.som",
"IntegerLoop.som", "List.som",  "Loop.som", "Permute.som", "Queens.som", "QuickSort.som",
"Recurse.som", "Sieve.som", "Storage.som", "Sum.som", "Towers.som", "TreeSort.som"]

VMS = [
"/bin/generational_badcache_noTagging",
"/bin/generational_nocache_noTagging",
"/bin/generational_nocache_tagging",
"/bin/generational_cache_noTagging",
"/bin/generational_cache_tagging",
"/bin/copying_badcache_noTagging",
"/bin/copying_nocache_noTagging",
"/bin/copying_nocache_tagging",
"/bin/copying_cache_noTagging",
"/bin/copying_cache_tagging",
"/bin/mark_sweep_badcache_noTagging",
"/bin/mark_sweep_nocache_noTagging",
"/bin/mark_sweep_nocache_tagging",
"/bin/mark_sweep_cache_noTagging",
"/bin/mark_sweep_cache_tagging"]

if __name__ == "__main__":
    if len(sys.argv) < 2:
        arguments = "SRGH"
    else:
        arguments = sys.argv[1]

    out_path = base_path+"/benchmark_results/"
    if not os.path.isdir(out_path):
        os.mkdir(out_path)

    for vm in VMS:
        vm_name = vm[vm.rfind("/")+1:]

        #SOM Benchmarks
        if "S" in arguments:
            runner = BenchmarkRunner(base_path + vm, ITERATIONS)
            for bm_name in SOM_BENCHMARKS:
                runner.add_benchmark("Examples/Benchmarks/"+bm_name)
            runner.run_benchmarks()
            csv_file = open(out_path + vm_name + "_som.csv","w")
            csv_file.write(runner.get_csv())
            csv_file.close()

        #Richards Benchmark
        if "R" in arguments:
            runner = BenchmarkRunner(base_path + vm, ITERATIONS)
            runner.add_benchmark("Examples/Benchmarks/Richards/RichardsBenchmarks.som")
            runner.run_benchmarks()
            csv_file = open(out_path + vm_name + "_richards.csv","w")
            csv_file.write(runner.get_csv())
            csv_file.close()

        #GC Bench
        if "G" in arguments:
            runner = BenchmarkRunner(base_path + vm, ITERATIONS)
            runner.add_benchmark("Examples/Benchmarks/GCBenchmark/GCBench.som")
            runner.run_benchmarks()
            csv_file = open(out_path + vm_name + "_gcbench.csv","w")
            csv_file.write(runner.get_csv())
            csv_file.close()

    if "H" in arguments:
        #SOM Benchmarks
        runner = BenchmarkRunner(base_path + "/bin/cppsom_statistics", 1)
        for bm_name in SOM_BENCHMARKS:
            runner.add_benchmark("Examples/Benchmarks/"+bm_name)
        runner.add_benchmark("Examples/Benchmarks/All.som")
        runner.run_benchmarks()

        #SOM Benchmarks
        runner = BenchmarkRunner(base_path + "/bin/cppsom_statistics", 1)
        runner.add_benchmark("Examples/Benchmarks/Richards/RichardsBenchmarks.som")
        runner.run_benchmarks()

        #now copy all generated csv files
        csv_files=[s for s in listdir("./") if s.endswith(".csv")]
        for f in csv_files:
            rename(f, out_path+f)


