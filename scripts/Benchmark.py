#! /usr/bin/python
import sys
import time
import subprocess
import re

regex_bm_time = re.compile(r"\{(\D*)(\d+)\s+\}", re.MULTILINE)
regex_gc_time = re.compile("\[(\d+.\d+)\]", re.MULTILINE)

class Benchmark(object):
    def __init__(self, directory, benchmark, iterations):
        print directory
        self.directory = directory
        self.benchmark = benchmark
        self.iterations = iterations

    def get_name(self):
        return self.benchmark[self.benchmark.rfind("/")+1 : self.benchmark.find(".")]

    def run(self):
        try:
            exe = "%s/SOM++ -cp %s/Smalltalk " % (self.directory, self.directory)
            cl = exe + " " + self.benchmark
            print "Executuing: ", self.get_name(),
            sys.stdout.flush()
            self.times = []
            self.gc_times = []

            for i in xrange(self.iterations):
                start_time=time.time()
                proc = subprocess.Popen(cl.split(), shell=False,
                    bufsize=2048,stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                    env={"LD_LIBRARY_PATH" : self.directory})
                data=proc.communicate()[0]
                self.times.append(time.time() - start_time)
                gc_res = regex_gc_time.search(data)
                self.gc_times.append(float(gc_res.groups(0)[0]))
                sys.stdout.write(".")
                sys.stdout.flush()
            print ""
        except OSError:
            raise Exception("Error when executing benchmark: " + cl)

    def get_csv(self):
        return self.get_name()+ ", " + ", ".join([str(t) for t in self.times])
