#!/bin/bash

init_submodule_and_path() {
  git submodule update --init
  if [ "$MACHINE" = "zullie1" ]; then
    export PATH=/opt/local/bin:/Users/gitlab-runner/Library/Python/3.12/bin:$PATH
    export MP='-mp'
  fi
  export PATH=$HOME/.local/bin:$PATH
}

prepare_build_folders() {
  rm -rf release && mkdir release
  rm -rf debug && mkdir debug
}

set_compiler() {
  if [ "$COMPILER" = "gcc" ];   then export CC=gcc-13;   export CXX=g++-13; fi
  if [ "$COMPILER" = "clang" ]; then export CC=clang$MP-18; export CXX=clang++$MP-18; fi
}

determine_name() {
  export NAME="som-$COMPILER-$GC"
  if [[ $CMAKE_FLAGS =~ "USE_TAGGING=true" ]]; then
    NAME="$NAME-inttag"
  else
    NAME="$NAME-intbox"
  fi
  
  if [[ $CMAKE_FLAGS =~ "CACHE_INTEGER=true" ]]; then
    NAME="$NAME-intcache"
  fi
  if [[ $CMAKE_FLAGS =~ "DUSE_VECTOR_PRIMITIVES=false" ]]; then
    NAME="$NAME-somvec"
  fi
  
  NAME=$( echo "$NAME" | tr '[:upper:]' '[:lower:]' )
}

make_and_test_debug_build() {
  cd debug || exit 1
  cmake .. $CMAKE_FLAGS -DGC_TYPE=$GC -DCMAKE_BUILD_TYPE=Debug
  make -j
  ./SOM++ -cfg -cp ../Smalltalk ../TestSuite/TestHarness.som
  ./unittests -cfg -cp ../Smalltalk:../TestSuite/BasicInterpreterTests ../Examples/Hello.som
  ./SOM++ -prim-hash-check -cp ../Smalltalk ../Examples/Benchmarks/BenchmarkHarness.som VectorBenchmark 1 1
  cd ..
}

make_and_test_release_build() {
  cd release || exit 1
  cmake .. $CMAKE_FLAGS -DGC_TYPE=$GC -DCMAKE_BUILD_TYPE=Release
  make -j
  ./SOM++ -cfg -cp ../Smalltalk ../TestSuite/TestHarness.som
  mv SOM++ ../$NAME
  cd ..
}

test_somsom() {
  ./$NAME -cp Smalltalk:TestSuite:core-lib/SomSom/src/compiler:core-lib/SomSom/src/interpreter:core-lib/SomSom/src/primitives:core-lib/SomSom/src/vm:core-lib/SomSom/src/vmobjects core-lib/SomSom/tests/SomSomTests.som
}

set_m() {
  if [ "$MACHINE" = "zullie1" ] || [ "$MACHINE" = "cassius" ]; then
    export M=''
  # for benchmarking we treat these machines like the yuria ones, just to
  # have some load balancing
  elif [ "$MACHINE" = "brutus" ]; then
    export M="t:yuria"
  elif [ "$MACHINE" = "laertes" ]; then
    export M="t:yuria2"
  elif [ "$MACHINE" = "ophelia" ]; then
    export M="t:yuria3"
  else
    export M="t:$MACHINE"
  fi
}
