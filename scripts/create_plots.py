#!/usr/bin/python
from os import listdir
from os import rename
from subprocess import call

if __name__=="__main__":
    files = filter(lambda x : x.endswith(".gp"), listdir("scripts"))
    for gp_file in files:
        src_file = "scripts/" + gp_file
        eps_file = gp_file[:gp_file.rfind(".")]+".eps"
        png_file = gp_file[:gp_file.rfind(".")]+".png"
        print "creating eps and png from %s" % src_file
        call(["gnuplot", src_file])
        call(["convert", "-density", "300", eps_file, png_file])
