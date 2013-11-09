#!/bin/sh
if [ "`uname`" = "Linux" ]
then
    export LD_LIBRARY_PATH=.
fi
./SOM++ "$@"