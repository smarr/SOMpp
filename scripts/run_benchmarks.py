#! /usr/bin/python
import os
import sys

from BenchmarkRunner import BenchmarkRunner
import os.path

base_path = os.path.abspath(os.path.dirname(sys.argv[0]) + "/../")

SOM_BENCHMARKS = [
"Bounce.som", "BubbleSort.som", "Dispatch.som", "Fibonacci.som",
"IntegerLoop.som", "List.som",  "Loop.som", "Permute.som", "Queens.som", "QuickSort.som",
"Recurse.som", "Sieve.som", "Storage.som", "Sum.som", "Towers.som", "TreeSort.som"]

VMS = [
"/bin/copying_badcache_noTagging",
"/bin/copying_nocache_noTagging",
"/bin/copying_nocache_tagging",
"/bin/generational_cache_noTagging",
"/bin/mark_sweep_badcache_noTagging",
"/bin/mark_sweep_nocache_noTagging",
"/bin/mark_sweep_nocache_tagging",
"/bin/copying_cache_noTagging",
"/bin/generational_badcache_noTagging",
"/bin/generational_nocache_noTagging",
"/bin/generational_nocache_tagging",
"/bin/mark_sweep_cache_noTagging"]

if __name__ == "__main__":
    out_path = base_path+"/benchmark_results/"
    if not os.path.isdir(out_path):
        os.mkdir(out_path)

    for vm in VMS:
        runner = BenchmarkRunner(base_path + vm, 20)
        for bm_name in SOM_BENCHMARKS:
            runner.add_benchmark("Examples/Benchmarks/"+bm_name)
        runner.run_benchmarks()

        vm_name = vm[vm.rfind("/")+1:]
        csv_file = open(out_path + vm_name + "_som.csv","w")
        csv_file.write(runner.get_csv())
        csv_file.close()


