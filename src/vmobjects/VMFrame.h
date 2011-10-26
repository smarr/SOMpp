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
#ifdef USE_TAGGING
#include "VMIntPointer.h"
#endif
class VMMethod;
class VMObject;
class VMInteger;


class VMFrame : public VMArray {
public:
    static pVMFrame EmergencyFrameFrom(pVMFrame from, int extraLength);

    VMFrame(int size, int nof = 0);
    
    inline pVMFrame   GetPreviousFrame() const;
    inline void       SetPreviousFrame(pVMObject);
    inline void       ClearPreviousFrame();
    bool       HasPreviousFrame() const;
    inline bool       IsBootstrapFrame() const;
    inline pVMFrame   GetContext() const;
    inline void       SetContext(pVMFrame);
    bool       HasContext() const;
    pVMFrame   GetContextLevel(int);
    pVMFrame   GetOuterContext();
    inline pVMMethod  GetMethod() const;
    void       SetMethod(pVMMethod);
    pVMObject  Pop();
    void       Push(pVMObject);
    void       ResetStackPointer();
    int        GetBytecodeIndex() const;
    void       SetBytecodeIndex(int);
    pVMObject  GetStackElement(int) const;
    void       SetStackElement(int, pVMObject);
    pVMObject  GetLocal(int, int);
    void       SetLocal(int, int, pVMObject);
    pVMObject GetArgument(int, int);
    void       SetArgument(int, int, pVMObject);
    void       PrintStackTrace() const;
    int        ArgumentStackIndex(int index) const;
    void       CopyArgumentsFrom(pVMFrame frame);
#ifdef USE_TAGGING
    virtual VMFrame*   Clone() const;
#else
    virtual pVMFrame   Clone() const;
#endif
    
    void       PrintStack() const;
    inline int32_t GetStackPointer() const;
    int        RemainingStackSize() const;
private:
    pVMFrame   previousFrame;
    pVMFrame   context;
    pVMMethod  method;
    pVMInteger stackPointer;
    pVMInteger bytecodeIndex;
    pVMInteger localOffset;

    static const int VMFrameNumberOfFields;
};

bool     VMFrame::IsBootstrapFrame() const {
    return !HasPreviousFrame();
}

pVMFrame VMFrame::GetContext() const {
    return this->context;
}

void     VMFrame::SetContext(pVMFrame frm) {
    this->context = frm;
    _HEAP->writeBarrier(this, frm);
}

int32_t VMFrame::GetStackPointer() const {
#ifdef USE_TAGGING
  return (int32_t)stackPointer;
#else
    return stackPointer->GetEmbeddedInteger();
#endif
}



pVMFrame VMFrame::GetPreviousFrame() const {
    return (pVMFrame) this->previousFrame;
}


void     VMFrame::SetPreviousFrame(pVMObject frm) {
    this->previousFrame = (pVMFrame)frm;
    _HEAP->writeBarrier(this, frm);
}

void     VMFrame::ClearPreviousFrame() {
    this->previousFrame = (pVMFrame)nilObject;
    _HEAP->writeBarrier(this, nilObject);
}

pVMMethod VMFrame::GetMethod() const {
    return this->method;
}
#endif
