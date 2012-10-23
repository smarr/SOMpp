#pragma once
#ifndef OBJECTFORMATS_H_
#define OBJECTFORMATS_H_

/*
 *
 *
Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
http://www.hpi.uni-potsdam.de/swa/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
  */
//some MACROS for integer tagging
/**
 * max value for tagged integers
 * 01111111 11111111 11111111 1111111X  
 */
#define VMTAGGEDINTEGER_MAX  1073741823

/**
 * min value for tagged integers
 * 10000000 00000000 00000000 0000000X  
 */
#define VMTAGGEDINTEGER_MIN -1073741824
#define AS_POINTER(X) ((AbstractVMObject*)X)

#ifdef ADDITIONAL_ALLOCATION
#define TAG_INTEGER(X) ((X >= VMTAGGEDINTEGER_MIN && X <= VMTAGGEDINTEGER_MAX && _UNIVERSE->NewInteger(0)) ? ((pVMInteger)((X << 1) | 1)) : (_UNIVERSE->NewInteger(X)))
#else
#define TAG_INTEGER(X) ((X >= VMTAGGEDINTEGER_MIN && X <= VMTAGGEDINTEGER_MAX) ? ((pVMInteger)((X << 1) | 1)) : (_UNIVERSE->NewInteger(X)))
#endif

#define UNTAG_INTEGER(X) (((long)X&1) ? ((long)X>>1) : (((VMInteger*)X)->GetEmbeddedInteger()))
#define IS_TAGGED(X) ((long)X&1)

#define pVMArray VMArray*
#define pVMBigInteger VMBigInteger*
#define pVMBlock VMBlock*
#define pVMClass VMClass*
#define pVMDouble VMDouble*
#define pVMEvaluationPrimitive VMEvaluationPrimitive*
#define pVMFrame VMFrame*
#define pVMInteger VMInteger* 
#define pVMInvokable VMInvokable*
#define pVMMethod VMMethod* 
#ifdef USE_TAGGING
#define pVMObject void*
#else
#define pVMObject AbstractVMObject*
#endif
#define pVMPrimitive VMPrimitive* 
#define pVMString VMString* 
#define pVMSymbol VMSymbol* 
#define VMOBJECT_PTR AbstractVMObject*
#endif OBJECTFORMATS_H_
