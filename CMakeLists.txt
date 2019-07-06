# Copyright (c) 2019 Stefan Marr <git@stefan-marr.de>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
cmake_minimum_required (VERSION 3.12)
project(SOMpp LANGUAGES CXX)

add_executable(SOM++ "")
add_executable(unittests "")

target_compile_features(SOM++     PRIVATE cxx_std_14)
target_compile_features(unittests PRIVATE cxx_std_14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fstack-protector-all")

set(ROOT_DIR         "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR		       "${ROOT_DIR}/src")
set(COMPILER_DIR 	   "${SRC_DIR}/compiler")
set(INTERPRETER_DIR  "${SRC_DIR}/interpreter")
set(MEMORY_DIR 		   "${SRC_DIR}/memory")
set(MISC_DIR 		     "${SRC_DIR}/misc")
set(VM_DIR 			     "${SRC_DIR}/vm")
set(VMOBJECTS_DIR 	 "${SRC_DIR}/vmobjects")
set(UNITTEST_DIR 	   "${SRC_DIR}/unitTests")

set(PRIMITIVES_DIR "${SRC_DIR}/primitives")
set(PRIMITIVESCORE_DIR "${SRC_DIR}/primitivesCore")

file(GLOB COMPILER_SRC    ${COMPILER_DIR}/*.cpp)
file(GLOB INTERPRETER_SRC ${INTERPRETER_DIR}/*.cpp)
file(GLOB MEMORY_SRC      ${MEMORY_DIR}/*.cpp)
file(GLOB MISC_SRC        ${MISC_DIR}/*.cpp)
file(GLOB VM_SRC          ${VM_DIR}/*.cpp)
file(GLOB VMOBJECTS_SRC   ${VMOBJECTS_DIR}/*.cpp)

file(GLOB MAIN_SRC           ${SRC_DIR}/*.cpp)
file(GLOB PRIMITIVES_SRC     ${PRIMITIVES_DIR}/*.cpp)
file(GLOB PRIMITIVESCORE_SRC ${PRIMITIVESCORE_DIR}/*.cpp)

file(GLOB UNITTEST_SRC    ${UNITTEST_DIR}/*.cpp)

option(USE_TAGGING "Enable immediate integers using tagging" FALSE)
set(GC_TYPE "COPYING" CACHE STRING "Select the type of GC to be used: COPYING, MARK_SWEEP, GENERATIONAL")

option(CACHE_INTEGER "Enable caching of boxed integers" FALSE)
set(INT_CACHE_MIN_VALUE -5  CACHE STRING "Lower bound of cached integers")
set(INT_CACHE_MAX_VALUE 100 CACHE STRING "Upper bound of cached integers")
option(GENERATE_INTEGER_HISTOGRAM "Generate histogram of allocated integers" FALSE)

option(GENERATE_ALLOCATION_STATISTICS "Generate allocation statistics" FALSE)

option(LOG_RECEIVER_TYPES        "Log types of receivers" FALSE)
option(UNSAFE_FRAME_OPTIMIZATION "Enable unsafe frame optimization" FALSE)
option(ADDITIONAL_ALLOCATION     "Enable additional allocations" FALSE)

option(FOR_PROFILING "Compile for profiling" FALSE)

option(USE_OMR_JIT "Enable OMR just-in-time compilation" FALSE)

if (USE_TAGGING)
  add_definitions(-DUSE_TAGGING)
  if (CACHE_INTEGER)
	  message(FATAL_ERROR "CACHE_INTEGER needs to be disabled when tagging is used.")
	endif ()
endif()

if (CACHE_INTEGER)
	add_definitions(
		-DCACHE_INTEGER
	 	-DINT_CACHE_MIN_VALUE=${INT_CACHE_MIN_VALUE}
		-DINT_CACHE_MAX_VALUE=${INT_CACHE_MAX_VALUE})
endif ()

if (GENERATE_INTEGER_HISTOGRAM)
  add_definitions(-DGENERATE_INTEGER_HISTOGRAM)
endif ()
if (GENERATE_ALLOCATION_STATISTICS)
  add_definitions(-DGENERATE_ALLOCATION_STATISTICS)
endif ()
if (UNSAFE_FRAME_OPTIMIZATION)
  add_definitions(-DUNSAFE_FRAME_OPTIMIZATION)
endif ()
if (LOG_RECEIVER_TYPES)
  add_definitions(-DLOG_RECEIVER_TYPES)
endif ()
if (ADDITIONAL_ALLOCATION)
  add_definitions(-DADDITIONAL_ALLOCATION)
endif ()

if (FOR_PROFILING)
  add_definitions(-g -pg)
endif ()

# OSX Hack for C++14
add_definitions(-D_XOPEN_SOURCE)



if (GC_TYPE EQUAL "OMR_GC" OR USE_OMR_JIT)
	set(USE_OMR true)
else()
  set(USE_OMR false)
endif()

if (USE_OMR)
  set(OMR_GLUE_DIR 	   "${SRC_DIR}/omrglue")

  add_subdirectory(omr)

  file(GLOB OMR_GLUE_SRC    ${OMR_GLUE_DIR}/*.cpp)
  set(OMR_INCLUDE
    "${ROOT_DIR}/omr/compiler"
    "${ROOT_DIR}/omr/jitbuilder/release/include"
    "${ROOT_DIR}/omr/gc"
    "${ROOT_DIR}/omr/gc/structs"
    "${ROOT_DIR}/omr/gc/include"
    "${ROOT_DIR}/omr/gc/startup"
    "${ROOT_DIR}/omr/gc/verbose"
    "${ROOT_DIR}/omr/gc/verbose/handler_standard"
    "${ROOT_DIR}/omr/gc/api"
    "${ROOT_DIR}/omr/gc/base"
    "${ROOT_DIR}/omr/gc/base/segregated"
    "${ROOT_DIR}/omr/gc/base/standard"
    "${ROOT_DIR}/omrglue"
    "${ROOT_DIR}/omr/gc/stats"
    "${ROOT_DIR}/omr/include_core"
    "${ROOT_DIR}/omr"
    "${CMAKE_BINARY_DIR}/omr"
  	"${CMAKE_BINARY_DIR}/omr/gc")
endif ()

if (GC_TYPE STREQUAL "OMR_GC")
  set(GC_TYPE "OMR_GARBAGE_COLLECTION")
endif ()

if (USE_OMR_JIT)
  set(DEFAULT_OMR_JIT_ON true)
  target_compile_definitions(SOM++ PRIVATE
	  -DDEFAULT_OMR_JIT_ON=true)
endif ()

add_definitions(-DGC_TYPE=${GC_TYPE})

target_sources(SOM++ PRIVATE
	${COMPILER_SRC}
	${INTERPRETER_SRC}
	${MEMORY_SRC}
	${MISC_SRC}
	${VM_SRC}
	${VMOBJECTS_SRC}

	${MAIN_SRC}
	${PRIMITIVES_SRC}
	${PRIMITIVESCORE_SRC})

if (USE_OMR)
  target_sources(SOM++ PRIVATE
    ${OMR_GLUE_SRC})
endif ()

target_compile_options(SOM++ PRIVATE
	-m64
	-Wno-endif-labels)

target_include_directories(SOM++ PRIVATE ${SRC_DIR})

if (USE_OMR)
  target_include_directories(SOM++ PRIVATE ${OMR_INCLUDE})
endif ()



target_link_libraries(SOM++)

if (USE_OMR)
  target_link_libraries(SOM++ omrcore omrgc omrvmstartup jitbuilder)
endif ()

target_sources(unittests PRIVATE
	${COMPILER_SRC}
	${INTERPRETER_SRC}
	${MEMORY_SRC}
	${MISC_SRC}
	${VM_SRC}
	${VMOBJECTS_SRC}

	${PRIMITIVES_SRC}
	${PRIMITIVESCORE_SRC}

	${UNITTEST_SRC})

target_compile_options(unittests PRIVATE
	-DDEBUG -O0 -g -DUNITTESTS)

target_include_directories(unittests PRIVATE
	${SRC_DIR}
	/opt/local/include)

if (USE_OMR)
  target_include_directories(unittests PRIVATE ${OMR_INCLUDE})
endif ()

find_library(LIB_CPPUNIT
	NAMES cppunit
	HINTS /opt/local/lib)

target_link_libraries(unittests ${LIB_CPPUNIT})

enable_testing()

add_test(
	NAME    unittests
	COMMAND unittests -cp ${ROOT_DIR}/Smalltalk ${ROOT_DIR}/Examples/Hello.som)

add_test(
	NAME som-tests
	COMMAND SOM++ -cp ${ROOT_DIR}/Smalltalk ${ROOT_DIR}/TestSuite/TestHarness.som)

add_test(
	NAME benchmarks
	COMMAND SOM++ -cp ${ROOT_DIR}/Smalltalk:${ROOT_DIR}/Examples/Benchmarks/LanguageFeatures ${ROOT_DIR}/Examples/Benchmarks/All.som)
