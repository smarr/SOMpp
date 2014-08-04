#pragma once
#ifndef VMFRAME_H_
#define VMFRAME_H_

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

#include "VMArray.h"
#include "VMObject.h"

//class VMObject;
class VMMethod;
class VMInteger;
class Universe;

class VMFrame: public VMObject {
    friend class Universe;
public:
    static pVMFrame EmergencyFrameFrom(pVMFrame from, long extraLength);

    VMFrame(long size, long nof = 0);

    inline pVMFrame GetPreviousFrame() /*const*/;
    inline void SetPreviousFrame(pVMFrame);
    inline void ClearPreviousFrame();
    inline bool HasPreviousFrame() /*const*/;
    inline bool IsBootstrapFrame() /*const*/;
    inline pVMFrame GetContext() /*const*/;
    inline void SetContext(pVMFrame);
    inline bool HasContext() /*const*/;
    pVMFrame GetContextLevel(long);
    pVMFrame GetOuterContext();
    inline pVMMethod GetMethod() /*const*/;
    void SetMethod(pVMMethod);
    pVMObject Pop();
    void Push(pVMObject);
    void ResetStackPointer();
    inline long GetBytecodeIndex() const;
    inline void SetBytecodeIndex(long);
    pVMObject GetStackElement(long) const;
    void SetStackElement(long, pVMObject);
    pVMObject GetLocal(long, long);
    void SetLocal(long, long, pVMObject);
    pVMObject GetArgument(long, long);
    void SetArgument(long, long, pVMObject);
    void PrintStackTrace() const;
    long ArgumentStackIndex(long index) /*const*/;
    void CopyArgumentsFrom(pVMFrame frame);
    inline  pVMObject GetField(long index) /*const*/;
    
    virtual pVMFrame Clone() /*const*/;
    
#if GC_TYPE==PAUSELESS
    virtual void MarkReferences(Worklist*);
#else
    virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));
#endif

    void PrintStack() const;
    inline void* GetStackPointer() const;
    long RemainingStackSize() const;
    
private:
    pVMFrame previousFrame;
    pVMFrame context;
    pVMMethod method;
    long bytecodeIndex;
    pVMObject* arguments;
    pVMObject* locals;
    pVMObject* stack_ptr;

    static const long VMFrameNumberOfFields;
};

pVMObject VMFrame::GetField(long index) /*const*/ {
    if (index==4)
#ifdef USE_TAGGING
    return TAG_INTEGER(bytecodeIndex);
#else
    return _UNIVERSE->NewInteger(bytecodeIndex);
#endif
    return VMObject::GetField(index);
}

bool VMFrame::HasContext() /*const*/ {
    //assert(this->context == NULL);
    PG_HEAP(ReadBarrier((void**)(&this->context)));
    return this->context != NULL;
}

bool VMFrame::HasPreviousFrame() /*const*/ {
    //assert(previousFrame == NULL);
    PG_HEAP(ReadBarrier((void**)(&this->previousFrame)));
    return this->previousFrame != NULL;
}

long VMFrame::GetBytecodeIndex() const {
    return bytecodeIndex;
}

void VMFrame::SetBytecodeIndex(long index) {
    bytecodeIndex = index;
}

bool VMFrame::IsBootstrapFrame() /*const*/ {
    return !HasPreviousFrame();
}

pVMFrame VMFrame::GetContext() /*const*/ {
    PG_HEAP(ReadBarrier((void**)(&this->context)));
    return this->context;
}

void VMFrame::SetContext(pVMFrame frm) {
    this->context = frm;
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, frm);
#endif
}

void* VMFrame::GetStackPointer() const {
    return stack_ptr;
}

pVMFrame VMFrame::GetPreviousFrame() /*const*/ {
    PG_HEAP(ReadBarrier((void**)(&this->previousFrame)));
    return this->previousFrame;
}

void VMFrame::SetPreviousFrame(pVMFrame frm) {
    this->previousFrame = frm;
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, AS_POINTER(frm));
#endif
}

void VMFrame::ClearPreviousFrame() {
    this->previousFrame = NULL;
}

pVMMethod VMFrame::GetMethod() /*const*/ {
    PG_HEAP(ReadBarrier((void**)(&this->method)));
    return this->method;
}

#endif
