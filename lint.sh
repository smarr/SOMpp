#!/bin/sh
CFLAGS+=' -fdiagnostics-absolute-paths'
# clang-tidy-mp-18 --config-file=.clang-tidy ./src/compiler/Lexer.cpp -- -I/opt/local/include/ -DGC_TYPE=COPYING -DUNITTESTS -fdiagnostics-absolute-paths

#  -header-filter=.* 
# src/**/*.cpp
clang-tidy-mp-18 --config-file=.clang-tidy src/**/*.cpp -- -I/opt/local/include/ -fdiagnostics-absolute-paths -DGC_TYPE=COPYING -DUNITTESTS
