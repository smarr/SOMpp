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

#include "VMArray.h"
#include "VMObject.h"
#include "VMInteger.h"


class VMMethod;
class Universe;

class VMFrame: public VMObject {
    friend class Universe;
public:
    typedef GCFrame Stored;
    
    static VMFrame* EmergencyFrameFrom(VMFrame* from, long extraLength);

    VMFrame(long size, long nof = 0);

    inline VMFrame* GetPreviousFrame();
    inline void SetPreviousFrame(VMFrame*);
    inline void ClearPreviousFrame();
    inline bool HasPreviousFrame();
    inline bool IsBootstrapFrame();
    inline VMFrame* GetContext();
    inline void SetContext(VMFrame*);
    inline bool HasContext();
    VMFrame* GetContextLevel(long);
    VMFrame* GetOuterContext();
    inline VMMethod* GetMethod();
    void SetMethod(VMMethod*);
    vm_oop_t Pop();
    void Push(vm_oop_t);
    void ResetStackPointer();
    inline long GetBytecodeIndex() const;
    inline void SetBytecodeIndex(long);
    vm_oop_t GetStackElement(long) const;
    vm_oop_t GetLocal(long, long);
    void SetLocal(long, long, vm_oop_t);
    vm_oop_t GetArgument(long, long);
    void SetArgument(long, long, vm_oop_t);
    void PrintStackTrace() const;
    long ArgumentStackIndex(long index);
    void CopyArgumentsFrom(VMFrame* frame);
    inline  vm_oop_t GetField(long index);
    
    virtual void MarkObjectAsInvalid();
    
#if GC_TYPE==PAUSELESS
    virtual VMFrame* Clone(Interpreter*);
    virtual VMFrame* Clone(PauselessCollectorThread*);
    virtual void MarkReferences();
    virtual void CheckMarking(void (vm_oop_t));
#else
    virtual VMFrame* Clone();
    virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));
#endif

    void PrintStack();
    inline void* GetStackPointer() const;
    long RemainingStackSize() const;
    
    virtual StdString AsDebugString();
    
private:
    GCFrame*  previousFrame;
    GCFrame*  context;
    GCMethod* method;
    long bytecodeIndex;
    gc_oop_t* arguments;
    gc_oop_t* locals;
    gc_oop_t* stack_ptr;

    static const long VMFrameNumberOfFields;
};

vm_oop_t VMFrame::GetField(long index) {
    return VMObject::GetField(index);
}

bool VMFrame::HasContext() {
    return load_ptr(this->context) != nullptr;
}

bool VMFrame::HasPreviousFrame() {
    return load_ptr(this->previousFrame) != nullptr;
}

long VMFrame::GetBytecodeIndex() const {
    return bytecodeIndex;
}

void VMFrame::SetBytecodeIndex(long index) {
    bytecodeIndex = index;
}

bool VMFrame::IsBootstrapFrame() {
    return !HasPreviousFrame();
}

VMFrame* VMFrame::GetContext() {
    return load_ptr(this->context);
}

void VMFrame::SetContext(VMFrame* frm) {
    this->context = store_ptr(frm);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, frm);
#endif
}

void* VMFrame::GetStackPointer() const {
    return stack_ptr;
}

VMFrame* VMFrame::GetPreviousFrame() {
    return load_ptr(this->previousFrame);
}

void VMFrame::SetPreviousFrame(VMFrame* frm) {
    this->previousFrame = store_ptr(frm);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, AS_VM_POINTER(frm));
#endif
}

void VMFrame::ClearPreviousFrame() {
    this->previousFrame = NULL;
}

VMMethod* VMFrame::GetMethod() {
    return load_ptr(this->method);
}
