SOM++ - The Simple Object Machine implemented in C++
====================================================

Introduction
------------

SOM is a minimal Smalltalk dialect used to teach VM construction at the [Hasso
Plattner Institute][SOM]. It was originally built at the University of Århus
(Denmark) where it was also used for teaching.

Currently, implementations exist for Java (SOM), C (CSOM), C++ (SOM++), Python
(PySOM), RPython (RPySOM), the Truffle framework (TruffleSOM), and
Squeak/Pharo Smalltalk (AweSOM).

A simple SOM Hello World looks like:

```Smalltalk
Hello = (
  run = (
    'Hello World!' println.
  )
)
```

This repository contains the C++ implementation of SOM, including an
implementation of the SOM standard library and a number of examples. Please see
the [main project page][SOMst] for links to the VM implementations.


SOM++ can be built with Make:

    $ make

Afterwards, the tests can be executed with:

    $ ./SOM++ -cp Smalltalk TestSuite/TestHarness.som
   
A simple Hello World program is executed with:

    $ ./SOM++ -cp Smalltalk Examples/Hello.som

**Note**: On Linux, the library search path needs to be adapted:

    $ export LD_LIBRARY_PATH=.

The debug version of CSOM can be built using the `debug` target:

    $ make debug

Information on previous authors are included in the AUTHORS file. This code is
distributed under the MIT License. Please see the LICENSE file for details.
Additional documentation, detailing for instance the object model and how to
implement primitives, is available in the `doc` folder.

Advanced Compilation Settings
-----------------------------

SOM++ supports different garbage collectors, including a basic mark/sweep, and
a generational GC. Furthermore, it implements different variants for integer
handling.


Tagged integers:

    default: off
    option name: TAGGING
    example: make TAGGING=true

Integer caching:

    default: off
    option name: INT_CACHE
    example: make INT_CACHE=true

Build Status
------------

Thanks to Travis CI, all commits of this repository are tested.
The current build status is: [![Build Status](https://travis-ci.org/SOM-st/SOMpp.png?branch=master)](https://travis-ci.org/SOM-st/SOMpp/)

 [SOM]: http://www.hpi.uni-potsdam.de/hirschfeld/projects/som/
 [SOMst]: https://travis-ci.org/SOM-st/

