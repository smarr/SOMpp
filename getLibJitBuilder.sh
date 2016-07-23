#!/bin/sh

# This file will download libjitbuilder that can be used for the automated
# travis CI testing.  It has only been tested on Ubuntu 14.04.  Use at your
# own risk and responsibility. Please use the docker image for playing with
# SOM++ and libjitbuilder.

if [ "${GC_TYPE}" = "omr_gc" ]; then
	rm -rf ../libjitbuilder
	mkdir ../libjitbuilder
	cd ../libjitbuilder
	wget https://ibm.box.com/shared/static/x4uhlh6rbu2tpth0p3i9bgaff1mf10u1.tgz
	tar -zxf x4uhlh6rbu2tpth0p3i9bgaff1mf10u1.tgz
	rm -f x4uhlh6rbu2tpth0p3i9bgaff1mf10u1.tgz
fi