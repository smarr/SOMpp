#!/bin/sh
clang-format-mp-18 -i --style=file --Werror src/*.cpp  src/**/*.cpp src/**/*.h
clang-tidy-mp-18 --config-file=.clang-tidy src/**/*.cpp -- \
  -I/opt/local/include/ -fdiagnostics-absolute-paths \
  -DGC_TYPE=GENERATIONAL -DUSE_TAGGING=false -DCACHE_INTEGER=false -DUNITTESTS
