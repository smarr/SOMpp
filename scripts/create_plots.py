#!/usr/bin/python
from os import listdir
from os import rename
from subprocess import call
import argparse

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='Run Benchmarks for all SOM++ VMs')
    parser.add_argument("output_types", nargs="*", default = ["eps"])
    args = parser.parse_args()

    files=[s for s in listdir("scripts") if s.endswith(".gp")]
    for gp_file in files:
        src_file = "scripts/" + gp_file
        eps_file = gp_file[:gp_file.rfind(".")]+".eps"
        png_file = gp_file[:gp_file.rfind(".")]+".png"
        print "creating %s from %s" % (" and ".join(args.output_types), src_file)
        if "eps" in args.output_types:
            call(["gnuplot", src_file])
        if "png" in args.output_types:
            call(["convert", "-density", "300", eps_file, png_file])
