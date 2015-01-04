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
 * 01111111 11111111 ... 11111111 1111111X
 *
 */
#define VMTAGGEDINTEGER_MAX  0x3FFFFFFFFFFFFFFF

/**
 * min value for tagged integers
 * 10000000 00000000 ... 00000000 0000000X
 */
#define VMTAGGEDINTEGER_MIN -0x4000000000000000

#if ADDITIONAL_ALLOCATION
#define TAG_INTEGER(X) (((X) >= VMTAGGEDINTEGER_MIN && (X) <= VMTAGGEDINTEGER_MAX && GetUniverse()->NewInteger(0)) ? ((vm_oop_t)(((X) << 1) | 1)) : (GetUniverse()->NewInteger(X)))
#else
#define TAG_INTEGER(X) (((X) >= VMTAGGEDINTEGER_MIN && (X) <= VMTAGGEDINTEGER_MAX) ? ((vm_oop_t)(((X) << 1) | 1)) : (GetUniverse()->NewInteger(X)))
#endif

#if USE_TAGGING
  #define INT_VAL(X) (IS_TAGGED(X) ? ((int64_t)(X)>>1) : (((VMInteger*)(X))->GetEmbeddedInteger()))
  #define NEW_INT(X) (TAG_INTEGER((X)))
  #define IS_TAGGED(X) ((int64_t)X&1)
  #define CLASS_OF(X) (IS_TAGGED(X)?load_ptr(integerClass):((AbstractVMObject*)(X))->GetClass())
  #define AS_OBJ(X) (IS_TAGGED(X)?GlobalBox::IntegerBox():((AbstractVMObject*)(X)))
#else
  #define INT_VAL(X) (static_cast<VMInteger*>(X)->GetEmbeddedInteger())
  #define NEW_INT(X) (GetUniverse()->NewInteger(X))
  #define IS_TAGGED(X) false
  #define CLASS_OF(X) (AS_OBJ(X)->GetClass())
  #define AS_OBJ(X) ((AbstractVMObject*)(X))
#endif


// Forward definitions of VM object classes
class AbstractVMObject;
class VMArray;
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

// VMOop and GCOop are classes to be able to type the pointer that can be
// tagged ints as well as AbstractVMObjects. Distinguish between stored
// pointers and loaded pointers, i.e., GCOop and VMOop respectively.
// Every GCAbstractObject can be directly stored as a GCOop.
// Integers need to be tagged first, but pointers don't.
// On the other hand, VMOops need to be untagged first to become useful.
class GCOop;
class VMOop {
    virtual void dummyVirtualFunctionToForceVTableCreation() {
        /* With the current class hierarchy, we need to force the compiler to
           create a VTable early, otherwise, the object layout is having
           vtables in the body of the objects, and casting is messed up, 
           leading to offset pointers to the vtables of subclasses. */ };
    public: typedef GCOop Stored; };
class GCOop { public: typedef VMOop Loaded; };

// oop_t: Ordinary Object Pointer type
// an oop_t can refer to tagged integers as well as normal AbstractVMObjects
typedef VMOop* vm_oop_t;
typedef GCOop* gc_oop_t;



/**
 We need to distinguish between pointers that need to be handled with a
 read barrier, and between pointers that already went through it.
 
 So, we call pointers that need to go through the barrier:
 heap values, or GC* pointers.
 
 And all the stuff that was already processed:
 loaded values, or VM* pointers.
 */
class GCAbstractObject : public GCOop    { public: typedef AbstractVMObject Loaded; };
class GCObject : public GCAbstractObject { public: typedef VMObject Loaded; };
class GCFrame  : public GCObject         { public: typedef VMFrame  Loaded; };
class GCClass  : public GCObject         { public: typedef VMClass  Loaded; };
class GCArray  : public GCObject         { public: typedef VMArray  Loaded; };
class GCBlock  : public GCObject         { public: typedef VMBlock  Loaded; };
class GCDouble : public GCAbstractObject { public: typedef VMDouble Loaded; };
class GCInteger : public GCAbstractObject { public: typedef VMInteger Loaded; };
class GCInvokable : public GCObject      { public: typedef VMInvokable Loaded; };
class GCMethod : public GCInvokable      { public: typedef VMMethod    Loaded; };
class GCPrimitive : public GCInvokable   { public: typedef VMPrimitive Loaded; };
class GCEvaluationPrimitive : public GCPrimitive { public: typedef VMEvaluationPrimitive Loaded; };
class GCString    : public GCAbstractObject { public: typedef VMString Loaded; };
class GCSymbol    : public GCString      { public: typedef VMSymbol Loaded; };



// Used to mark object fields as invalid
#define INVALID_VM_POINTER ((VMObject*)0x101010)
#define INVALID_GC_POINTER ((GCObject*)0x101010)


template<typename T>
inline typename T::Loaded* load_ptr(T* gc_val) {
    return (typename T::Loaded*) gc_val;
}

template<typename T>
inline typename T::Stored* _store_ptr(T* vm_val) {
    return (typename T::Stored*) vm_val;
}


#define store_ptr(field, val) field = _store_ptr(val); write_barrier(this, val)

typedef gc_oop_t (*walk_heap_fn)(gc_oop_t);


