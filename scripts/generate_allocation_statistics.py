#!/usr/bin/python
from os import listdir
from os import rename
from subprocess import call

if __name__=="__main__":
    files=[s for s in listdir("benchmark_results") if
            s.endswith("allocation_statistics.csv")]
    for csv_file in files:
        name = csv_file[:csv_file.find("_")]
        csv_file = "benchmark_results/"+csv_file
        out_file = "allocation_statistics_no_" + name + ".eps"
        command = "sed 's/bench_name/%s/g;s/csv_file/%s/g;s/out_file/%s/g' < scripts/allocation_statistics_no_template.gpl | gnuplot" % (name, csv_file.replace("/","\/"), out_file)
        print "Executing: %s" % command
        call(command, shell=True)
        png_file = out_file.replace(".eps", ".png")
        call(["convert", "-density", "300", out_file, png_file])
        out_file = "allocation_statistics_size_" + name + ".eps"
        command = "sed 's/bench_name/%s/g;s/csv_file/%s/g;s/out_file/%s/g' < scripts/allocation_statistics_size_template.gpl | gnuplot" % (name, csv_file.replace("/","\/"), out_file)
        print "Executing: %s" % command
        call(command, shell=True)
        png_file = out_file.replace(".eps", ".png")
        call(["convert", "-density", "300", out_file, png_file])
