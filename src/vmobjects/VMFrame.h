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

class Universe;

class VMFrame: public VMObject {
    friend class Universe;
public:
    static pVMFrame EmergencyFrameFrom(pVMFrame from, long extraLength);

    VMFrame(long size, long nof = 0);

    inline pVMFrame GetPreviousFrame() const;
    inline void SetPreviousFrame(pVMFrame);
    inline void ClearPreviousFrame();
    inline bool HasPreviousFrame() const;
    inline bool IsBootstrapFrame() const;
    inline pVMFrame GetContext() const;
    inline void SetContext(pVMFrame);
    inline bool HasContext() const;
    pVMFrame GetContextLevel(long);
    pVMFrame GetOuterContext();
    inline pVMMethod GetMethod() const;
    void SetMethod(pVMMethod);
    oop_t Pop();
    void Push(oop_t);
    void ResetStackPointer();
    inline long GetBytecodeIndex() const;
    inline void SetBytecodeIndex(long);
    oop_t GetStackElement(long) const;
    oop_t GetLocal(long, long);
    void SetLocal(long, long, oop_t);
    oop_t GetArgument(long, long);
    void SetArgument(long, long, oop_t);
    void PrintStackTrace() const;
    long ArgumentStackIndex(long index) const;
    void CopyArgumentsFrom(pVMFrame frame);
    inline  oop_t GetField(long index) const;
    virtual void WalkObjects(oop_t (oop_t));
    virtual pVMFrame Clone() const;

    void PrintStack() const;
    inline void* GetStackPointer() const;
    long RemainingStackSize() const;
private:
    pVMFrame previousFrame;
    pVMFrame context;
    pVMMethod method;
    long bytecodeIndex;
    oop_t* arguments;
    oop_t* locals;
    oop_t* stack_ptr;

    static const long VMFrameNumberOfFields;
};

oop_t VMFrame::GetField(long index) const {
    if (index == 4)
        return NEW_INT(bytecodeIndex);
    return VMObject::GetField(index);
}

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

pVMFrame VMFrame::GetContext() const {
    return context;
}

void VMFrame::SetContext(pVMFrame frm) {
    context = frm;
    write_barrier(this, frm);
}

void* VMFrame::GetStackPointer() const {
    return stack_ptr;
}

pVMFrame VMFrame::GetPreviousFrame() const {
    return previousFrame;
}

void VMFrame::SetPreviousFrame(pVMFrame frm) {
    previousFrame = frm;
    write_barrier(this, frm);
}

void VMFrame::ClearPreviousFrame() {
    previousFrame = nullptr;
}

pVMMethod VMFrame::GetMethod() const {
    return method;
}
