#!/usr/bin/env make -f

#
#  Based on Makefile by Tobias Pape
#
#
# Copyright (c) 2007 Michael Haupt, Tobias Pape
# Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
# http://www.hpi.uni-potsdam.de/swa/
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

CXX		?=clang++
CFLAGS	=-std=c++11 -m64 -Wno-endif-labels $(OPT_FLAGS) $(DBG_FLAGS) $(FEATURE_FLAGS) $(INCLUDES)
OPT_FLAGS?=-O3 -DNDEBUG

LBITS := $(shell getconf LONG_BIT)
ARCH := $(shell arch)
ifeq ($(LBITS),64)
	CFLAGS += -fno-PIC -mcmodel=large
endif
ifeq ($(ARCH),armv7l)
        #https://bugs.launchpad.net/ubuntu/+source/gcc-4.4/+bug/503448/comments/12
	CFLAGS += -mword-relocations
endif

SHAREDFLAGS = -shared

LIBRARIES	=-L$(ROOT_DIR) -lrt
LDFLAGS		=$(DBG_FLAGS) $(LIBRARIES)

INSTALL		=install

CSOM_LIBS	=-lm

CSOM_NAME	=SOM++

include $(BUILD_DIR)/sources.make

#
#  metarules
#

.PHONY: clean clobber test

include $(BUILD_DIR)/config.make

all: $(CSOM_NAME)

debug : OPT_FLAGS=
debug : DBG_FLAGS=-DDEBUG -O0 -g
debug: all

profiling : DBG_FLAGS=-g -pg
profiling : LDFLAGS+=-pg
profiling: all


.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $*.o

clean:
	rm -Rf $(CLEAN)
	#just to be sure delete again
	find . -name "*.o" -delete

#
# product rules
#

$(CSOM_NAME): $(ALL_OBJ)
	@echo Linking $(CSOM_NAME)
	$(CXX) -o $(CSOM_NAME) $(ALL_OBJ) $(LDFLAGS) $(CSOM_LIBS)
	@echo Linking $(CSOM_NAME) done.

install: all
	@echo installing CSOM into build
	$(INSTALL) -d $(DEST_DIR)
	$(INSTALL) $(CSOM_NAME) $(DEST_DIR)
	@echo CSOM.
	cp -Rpf $(ST_DIR) $(EX_DIR) $(TEST_DIR)  $(DEST_DIR)
	@echo Library.
	@echo done.

#
# console: start the console
#
console: all
	./$(CSOM_NAME) -cp ./Smalltalk

units: $(ALL_TEST_OBJ)
	$(CXX) $(LIBRARIES) $(ALL_TEST_OBJ) -lcppunit -lrt -o unittest

richards: all
	./$(CSOM_NAME) -cp ./Smalltalk ./Examples/Benchmarks/Richards/RichardsBenchmarks.som

unittests : OPT_FLAGS=
unittests : DBG_FLAGS=-DDEBUG -O0 -g -DUNITTESTS
unittests: all units
	./unittest -cp ./Smalltalk ./Examples/Hello.som

#
# test: run the standard test suite
#
test: all
	./$(CSOM_NAME) -cp ./Smalltalk ./TestSuite/TestHarness.som

#
# bench: run the benchmarks
#
bench: all
	./$(CSOM_NAME) -cp ./Smalltalk:./Examples/Benchmarks/LanguageFeatures ./Examples/Benchmarks/All.som
