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
cmake_minimum_required (VERSION 3.20)
project(SOMpp LANGUAGES CXX)

set(CMAKE_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)

set(ROOT_DIR         "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR          "${ROOT_DIR}/src")
set(COMPILER_DIR     "${SRC_DIR}/compiler")
set(INTERPRETER_DIR  "${SRC_DIR}/interpreter")
set(MEMORY_DIR       "${SRC_DIR}/memory")
set(MISC_DIR         "${SRC_DIR}/misc")
set(VM_DIR           "${SRC_DIR}/vm")
set(VMOBJECTS_DIR    "${SRC_DIR}/vmobjects")
set(UNITTEST_DIR     "${SRC_DIR}/unitTests")

set(PRIMITIVES_DIR       "${SRC_DIR}/primitives")
set(PRIMITIVESCORE_DIR   "${SRC_DIR}/primitivesCore")

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

set(SOM_SRC "")
list(APPEND SOM_SRC
  ${COMPILER_SRC}
  ${INTERPRETER_SRC}
  ${MEMORY_SRC}
  ${MISC_SRC}
  ${VM_SRC}
  ${VMOBJECTS_SRC}

  ${MAIN_SRC}
  ${PRIMITIVES_SRC}
  ${PRIMITIVESCORE_SRC}
)

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

# Options to control PGO
option(ENABLE_PGO "Enable Profile-Guided Optimization" OFF)
option(PGO_PHASE "PGO phase (GENERATE, USE, or AUTO)" "")

# Path for profile data
set(PROFILE_DATA_DIR "${CMAKE_BINARY_DIR}/pgo_data")
set(PROFILE_RAW_FILE "${PROFILE_DATA_DIR}/prof-%p.profraw")
set(PROFILE_RAW_FILE_COLLECT "${PROFILE_DATA_DIR}/prof-*.profraw")
set(PROFILE_DATA_FILE "${PROFILE_DATA_DIR}/SOM++.profdata")

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

add_definitions(-DGC_TYPE=${GC_TYPE})

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-fdiagnostics-absolute-paths)
endif()



add_executable(SOM++ "")

target_sources(SOM++ PRIVATE ${SOM_SRC})

target_compile_options(SOM++ PRIVATE
  -m64
  -Wno-endif-labels)

target_include_directories(SOM++ PRIVATE ${SRC_DIR})

target_link_libraries(SOM++)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(SOM++ PRIVATE -O3 -flto)
  target_link_options(SOM++ PRIVATE -flto)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_executable(unittests "")

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

  add_definitions(-DDEBUG -DUNITTESTS)

  target_compile_options(unittests PRIVATE -O0 -g)

  target_include_directories(unittests PRIVATE
    ${SRC_DIR}
    /opt/local/include)

  find_library(LIB_CPPUNIT
    NAMES cppunit
    HINTS /opt/local/lib)

  target_link_libraries(unittests ${LIB_CPPUNIT})
endif()


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

function(extract_compiler_suffix compiler_name output_var)
    get_filename_component(name_only "${compiler_name}" NAME)

    if(name_only MATCHES "[^-]+(-.+)$")
        set(${output_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        message(STATUS "Detected compiler suffix: ${CMAKE_MATCH_1}")
    else()
        set(${output_var} "" PARENT_SCOPE)
        message(STATUS "No version suffix detected for compiler: ${name_only}")
    endif()
endfunction()

# Configure PGO if enabled
if(ENABLE_PGO)
    # Determine the correct llvm-profdata tool for Clang
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Extract version suffix from Clang compiler path
        get_filename_component(COMPILER_NAME "${CMAKE_CXX_COMPILER}" NAME)
        extract_compiler_suffix("${COMPILER_NAME}" COMPILER_SUFFIX)
        set(PROFDATA_BASE_NAME "llvm-profdata")
        set(PROFDATA_TOOL "${PROFDATA_BASE_NAME}${COMPILER_SUFFIX}")

        find_program(LLVM_PROFDATA ${PROFDATA_TOOL})
        if(NOT LLVM_PROFDATA)
            find_program(LLVM_PROFDATA "llvm-profdata")
        endif()

        if(NOT LLVM_PROFDATA)
            set(LLVM_PROFDATA "${PROFDATA_TOOL}")
            message(WARNING "Could not find ${PROFDATA_TOOL}, defaulting to ${LLVM_PROFDATA}")
        endif()

        message(STATUS "Using LLVM profdata tool: ${LLVM_PROFDATA}")
    endif()


    # Create directory for profile data
    file(MAKE_DIRECTORY ${PROFILE_DATA_DIR})

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Build with PGO using Clang")

        # First, create the instrumented build
        target_compile_options(SOM++ PRIVATE -fprofile-instr-generate)
        target_link_options(SOM++ PRIVATE -fprofile-instr-generate)

        add_custom_command(
            TARGET SOM++ POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${PROFILE_RAW_FILE} ${CMAKE_BINARY_DIR}/SOM++ -cp ${ROOT_DIR}/core-lib/Smalltalk ${ROOT_DIR}/core-lib/TestSuite/TestHarness.som
            COMMAND rebench -R -c ${ROOT_DIR}/rebench.conf SOM++pgo-training
            COMMENT "Running Tests and ReBench to collect profile data"
            WORKING_DIRECTORY ${ROOT_DIR}
        )

        add_custom_command(
            TARGET SOM++ POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${LLVM_PROFDATA} merge -output=${PROFILE_DATA_FILE} ${PROFILE_RAW_FILE_COLLECT}
            COMMENT "Running instrumented binary to generate profile data"
        )

        # Create optimized version of the app
        add_executable(SOM++opt ${SOM_SRC})
        target_include_directories(SOM++opt PRIVATE include)
        target_compile_options(SOM++opt PRIVATE -fprofile-instr-use=${PROFILE_DATA_FILE})
        target_link_options(SOM++opt PRIVATE -fprofile-instr-use=${PROFILE_DATA_FILE})

        # Make the optimized version depend on the instrumented version
        add_dependencies(SOM++opt SOM++)

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # PGO with GCC
        message(STATUS "Build with PGO using GCC: ${CMAKE_BINARY_DIR}/SOM++")

        # First, create the instrumented build
        target_compile_options(SOM++ PRIVATE -fprofile-generate=${PROFILE_DATA_DIR})
        target_link_options(SOM++ PRIVATE -fprofile-generate=${PROFILE_DATA_DIR})

        add_custom_command(
            TARGET SOM++ POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${PROFILE_RAW_FILE} ${CMAKE_BINARY_DIR}/SOM++ -cp ${ROOT_DIR}/core-lib/Smalltalk ${ROOT_DIR}/core-lib/TestSuite/TestHarness.som
            COMMAND rebench -R -c ${ROOT_DIR}/rebench.conf SOM++pgo-training
            COMMENT "Running Tests and ReBench to collect profile data"
            WORKING_DIRECTORY ${ROOT_DIR}
        )

        # Create optimized version of the app
        add_executable(SOM++opt ${SOM_SRC})
        target_include_directories(SOM++opt PRIVATE include)
        target_compile_options(SOM++opt PRIVATE -fprofile-use=${PROFILE_DATA_DIR})
        target_link_options(SOM++opt PRIVATE -fprofile-use=${PROFILE_DATA_DIR})

        # Make the optimized version depend on the instrumented version
        add_dependencies(SOM++opt SOM++)
    endif()
endif()
