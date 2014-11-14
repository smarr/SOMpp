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
#define AS_VM_POINTER(X) ((AbstractVMObject*)X)
#define AS_GC_POINTER(X) ((GCAbstractObject*)X)

#ifdef ADDITIONAL_ALLOCATION
#define TAG_INTEGER(X) ((X >= VMTAGGEDINTEGER_MIN && X <= VMTAGGEDINTEGER_MAX && _UNIVERSE->NewInteger(0)) ? ((pVMInteger)((X << 1) | 1)) : (_UNIVERSE->NewInteger(X)))
#else
#define TAG_INTEGER(X) ((X >= VMTAGGEDINTEGER_MIN && X <= VMTAGGEDINTEGER_MAX) ? ((pVMInteger)((X << 1) | 1)) : (_UNIVERSE->NewInteger(X)))
#endif

#define UNTAG_INTEGER(X) (((long)X&1) ? ((long)X>>1) : (((VMInteger*)X)->GetEmbeddedInteger()))
#define IS_TAGGED(X) ((long)X&1)


class AbstractVMObject;
class VMObject;
class VMFrame;
class VMClass;
class VMArray;
class VMBlock;
class VMDouble;
class VMBigInteger;
class VMInteger;
class VMEvaluationPrimitive;
class VMInvokable;
class VMMethod;
class VMPrimitive;
class VMString;
class VMSymbol;

class VMMutex;
class VMSignal;
class VMThread;

/**
 We need to distinguish between pointers that need to be handled with a
 read barrier, and between pointers that already went through it.
 
 So, we call pointers that need to go through the barrier:
    heap values, or GC* pointers.
 
 And all the stuff that was already processed:
    loaded values, or VM* pointers.
*/
class GCAbstractObject                   { public: typedef AbstractVMObject Loaded; };
class GCObject : public GCAbstractObject { public: typedef VMObject Loaded; };
class GCFrame  : public GCObject         { public: typedef VMFrame  Loaded; };
class GCClass  : public GCObject         { public: typedef VMClass  Loaded; };
class GCArray  : public GCObject         { public: typedef VMArray  Loaded; };
class GCBlock  : public GCObject         { public: typedef VMBlock  Loaded; };
class GCBigInteger : public GCAbstractObject { public: typedef VMBigInteger Loaded; };
class GCDouble : public GCAbstractObject { public: typedef VMDouble Loaded; };
class GCInteger : public GCAbstractObject { public: typedef VMInteger Loaded; };
class GCInvokable : public GCObject      { public: typedef VMInvokable Loaded; };
class GCMethod : public GCInvokable      { public: typedef VMMethod    Loaded; };
class GCPrimitive : public GCInvokable   { public: typedef VMPrimitive Loaded; };
class GCEvaluationPrimitive : public GCPrimitive { public: typedef VMEvaluationPrimitive Loaded; };
class GCString    : public GCAbstractObject { public: typedef VMString Loaded; };
class GCSymbol    : public GCString      { public: typedef VMSymbol Loaded; };
class GCMutex     : public GCObject      { public: typedef VMMutex Loaded; };
class GCSignal    : public GCObject      { public: typedef VMSignal Loaded; };
class GCThread    : public GCObject      { public: typedef VMThread Loaded; };

#define pVMThread VMThread*
#define pVMMutex VMMutex*
#define pVMSignal VMSignal*
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

// Used to mark object fields as invalid
#define INVALID_VM_POINTER ((pVMObject)0x101010)
#define INVALID_GC_POINTER ((GCObject*)0x101010)

#endif // OBJECTFORMATS_H_
