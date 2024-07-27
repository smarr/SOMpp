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

#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMMethod.h"

class Universe;

class VMFrame: public VMObject {
    friend class Universe;
public:
    typedef GCFrame Stored;

    static VMFrame* EmergencyFrameFrom(VMFrame* from, long extraLength);

    explicit VMFrame(long size, long nof = 0);

    inline VMFrame* GetPreviousFrame() const;
    inline void SetPreviousFrame(VMFrame*);
    inline void ClearPreviousFrame();
    inline bool HasPreviousFrame() const;
    inline bool IsBootstrapFrame() const;
    inline VMFrame* GetContext() const;
    inline void SetContext(VMFrame*);
    inline bool HasContext() const;
    VMFrame* GetContextLevel(long);
    VMFrame* GetOuterContext();
    inline VMMethod* GetMethod() const;
    void SetMethod(VMMethod*);

    inline vm_oop_t Pop() {
        vm_oop_t result = load_ptr(*stack_ptr);
        stack_ptr--;
        return result;
    }

    inline void Push(vm_oop_t obj) {
        assert(RemainingStackSize() > 0);
        ++stack_ptr;
        store_ptr(*stack_ptr, obj);
    }

    void ResetStackPointer() {
        // arguments are stored in front of local variables
        VMMethod* meth = GetMethod();
        locals = arguments + meth->GetNumberOfArguments();
        // Set the stack pointer to its initial value thereby clearing the stack
        stack_ptr = locals + meth->GetNumberOfLocals() - 1;
    }

    inline long GetBytecodeIndex() const;
    inline void SetBytecodeIndex(long);

    inline vm_oop_t GetStackElement(long index) const {
        return load_ptr(stack_ptr[-index]);
    }

    inline vm_oop_t GetLocal(long index, long contextLevel) {
        VMFrame* context = GetContextLevel(contextLevel);
        return load_ptr(context->locals[index]);
    }

    inline vm_oop_t GetLocalInCurrentContext(uint8_t localIndex) {
        return load_ptr(this->locals[localIndex]);
    }

    void SetLocal(long index, long context_level, vm_oop_t);

    inline void SetLocal(long index, vm_oop_t value) {
        store_ptr(locals[index], value);
    }

    inline vm_oop_t GetArgument(long index, long contextLevel) {
        // get the context
        VMFrame* context = GetContextLevel(contextLevel);
        return load_ptr(context->arguments[index]);
    }

    inline vm_oop_t GetArgumentInCurrentContext(long index) {
        return load_ptr(this->arguments[index]);
    }

    void SetArgument(long, long, vm_oop_t);
    void PrintStackTrace() const;
    long ArgumentStackIndex(long index) const;
    void CopyArgumentsFrom(VMFrame* frame);
    void WalkObjects(walk_heap_fn) override;
    VMFrame* Clone() const override;

    void PrintStack() const;
    void PrintBytecode() const;

    long RemainingStackSize() const;

    StdString AsDebugString() const override;

private_testable:
    long bytecodeIndex;

private:
    GCFrame* previousFrame;
    GCFrame* context;
    GCMethod* method;
    gc_oop_t* arguments;
    gc_oop_t* locals;
    gc_oop_t* stack_ptr;

    inline void SetArgument(long index, vm_oop_t value);

    static const long VMFrameNumberOfFields;
};

bool VMFrame::HasContext() const {
    return context != nullptr;
}

bool VMFrame::HasPreviousFrame() const {
    return previousFrame != nullptr;
}

long VMFrame::GetBytecodeIndex() const {
    return bytecodeIndex;
}

void VMFrame::SetBytecodeIndex(long index) {
    bytecodeIndex = index;
}

bool VMFrame::IsBootstrapFrame() const {
    return !HasPreviousFrame();
}

VMFrame* VMFrame::GetContext() const {
    return load_ptr(context);
}

void VMFrame::SetContext(VMFrame* frm) {
    store_ptr(context, frm);
}

VMFrame* VMFrame::GetPreviousFrame() const {
    return load_ptr(previousFrame);
}

void VMFrame::SetPreviousFrame(VMFrame* frm) {
    store_ptr(previousFrame, frm);
}

void VMFrame::ClearPreviousFrame() {
    previousFrame = nullptr;
}

VMMethod* VMFrame::GetMethod() const {
    return load_ptr(method);
}

void VMFrame::SetArgument(long index, vm_oop_t value) {
    store_ptr(arguments[index], value);
}
