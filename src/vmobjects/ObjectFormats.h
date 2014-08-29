#pragma once

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
#define TAG_INTEGER(X) ((X >= VMTAGGEDINTEGER_MIN && X <= VMTAGGEDINTEGER_MAX && GetUniverse()->NewInteger(0)) ? ((pVMInteger)((X << 1) | 1)) : (GetUniverse()->NewInteger(X)))
#else
#define TAG_INTEGER(X) ((X >= VMTAGGEDINTEGER_MIN && X <= VMTAGGEDINTEGER_MAX) ? ((pVMInteger)((X << 1) | 1)) : (GetUniverse()->NewInteger(X)))
#endif

#ifdef USE_TAGGING
//#define UNTAG_INTEGER(X) (((long)X&1) ? ((long)X>>1) : (((VMInteger*)X)->GetEmbeddedInteger()))
  #define INT_VAL(X) (((long)(X)&1) ? ((long)(X)>>1) : (((VMInteger*)(X))->GetEmbeddedInteger()))
  #define NEW_INT(X) (TAG_INTEGER((X)))
#else
  #define INT_VAL(X) (static_cast<pVMInteger>(X)->GetEmbeddedInteger())
  #define NEW_INT(X) (GetUniverse()->NewInteger(X))
#endif
#define IS_TAGGED(X) ((long)X&1)

// Forward definitions of VM object classes
class AbstractVMObject;
class VMArray;
class VMBigInteger;
class VMBlock;
class VMClass;
class VMDouble;
class VMEvaluationPrimitive;
class VMFrame;
class VMInteger;
class VMInvokable;
class VMMethod;
class VMObject;
class VMPrimitive;
class VMString;
class VMSymbol;

typedef VMArray*         pVMArray;
typedef const VMArray*  pcVMArray;

typedef VMBigInteger*   pVMBigInteger;
typedef VMBlock*        pVMBlock;
typedef VMClass*        pVMClass;
typedef const VMClass*  pcVMClass;
typedef VMDouble*       pVMDouble;

typedef VMEvaluationPrimitive* pVMEvaluationPrimitive;
typedef VMFrame*               pVMFrame;
typedef VMInteger*             pVMInteger;
typedef VMInvokable*           pVMInvokable;
typedef VMMethod*              pVMMethod;

#ifdef USE_TAGGING
  typedef void*                pVMObject;
  typedef const void*         pcVMObject;
#else
  typedef AbstractVMObject*    pVMObject;
  typedef const AbstractVMObject* pcVMObject;
#endif

typedef VMPrimitive*           pVMPrimitive;
typedef VMString*              pVMString;
typedef VMSymbol*              pVMSymbol;

typedef AbstractVMObject*      VMOBJECT_PTR;

// Used to mark object fields as invalid
#define INVALID_POINTER ((pVMObject)0x101010)
