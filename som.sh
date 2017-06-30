#!/bin/bash
pushd `dirname $0` > /dev/null
SCRIPT_PATH=`pwd`
popd > /dev/null

if [ "`uname`" = "Linux" ]
then
    export LD_LIBRARY_PATH=${SCRIPT_PATH}/
fi
${SCRIPT_PATH}/SOM++ "$@"
