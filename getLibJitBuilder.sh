#!/bin/sh

# This file will download libjitbuilder that can be used for the automated
# travis CI testing.  It has only been tested on Ubuntu 14.04.  Use at your
# own risk and responsibility. Please use the docker image for playing with
# SOM++ and libjitbuilder.

if [ "${GC_TYPE}" = "omr_gc" ]; then
	rm -rf ../libjitbuilder
	mkdir ../libjitbuilder
	cd ../libjitbuilder
	wget https://ibm.box.com/shared/static/gv0p9iqavz2dgozoipqh4ut0usbyys6q.tgz
	tar -zxf gv0p9iqavz2dgozoipqh4ut0usbyys6q.tgz
	rm -f gv0p9iqavz2dgozoipqh4ut0usbyys6q.tgz
fi