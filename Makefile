#!/usr/bin/env make -f

#
#  Makefile by Tobias Pape 
# $Id: Makefile 206 2008-04-14 12:22:39Z michael.haupt $
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
# 

export ROOT_DIR :=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
export BUILD_DIR:=$(ROOT_DIR)/build
export SRC_DIR	:=$(ROOT_DIR)/src
export LIB_DIR	:=$(ROOT_DIR)/lib

export ST_DIR	=$(ROOT_DIR)/core-lib/Smalltalk
export EX_DIR	=$(ROOT_DIR)/core-lib/Examples
export TEST_DIR	=$(ROOT_DIR)/core-lib/TestSuite

ifeq ($(OS),)
# only Windows has OS predefined.
	UNAME		:= $(shell uname -s)
else
	UNAME		:=windows
endif

#############
include $(BUILD_DIR)/$(UNAME).make
