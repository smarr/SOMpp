#!/bin/sh

# build.sh
# SOM
#
# Created by Robert Krahn on 4/27/09.
#
# Unfortunately the ACTION variable cannot be set inside XCode so we
# have to deal with make parameters like clean or debug in this script
#

if [ "$ACTION" = "clean" ] ; then
	make $ACTION
else
	make $1
fi