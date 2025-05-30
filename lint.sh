#!/bin/sh

CLANG_FORMAT="clang-format-mp-19 -i --style=file --Werror src/*.cpp  src/**/*.cpp src/**/*.h"
$CLANG_FORMAT

## The check --enable-check-profile flag gives an overview of where clang-tidy spends its time
# --enable-check-profile

CLANG_START="clang-tidy-mp-19  --config-file=.clang-tidy"
CLANG_END="--fix \
  -- -I/opt/local/include/ -fdiagnostics-absolute-paths \
  -DGC_TYPE=GENERATIONAL -DUSE_TAGGING=false -DCACHE_INTEGER=false -DUNITTESTS"


$CLANG_START src/*.cpp $CLANG_END  &
$CLANG_START src/compiler/*.cpp $CLANG_END  &
$CLANG_START src/interpreter/*.cpp $CLANG_END  &
$CLANG_START src/memory/*.cpp $CLANG_END  &
$CLANG_START src/misc/*.cpp $CLANG_END  &
$CLANG_START src/primitives/*.cpp $CLANG_END  &
$CLANG_START src/primitivesCore/*.cpp $CLANG_END  &
$CLANG_START src/unitTests/*.cpp $CLANG_END  &
$CLANG_START src/vm/*.cpp $CLANG_END  &
$CLANG_START src/vmobjects/*.cpp $CLANG_END  &

wait
$CLANG_FORMAT

