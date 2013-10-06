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
class VMMethod;
class VMObject;
class VMInteger;

class VMFrame: public VMArray {
public:
    static pVMFrame EmergencyFrameFrom(pVMFrame from, long extraLength);

    VMFrame(long size, long nof = 0);

    inline pVMFrame  GetPreviousFrame() const;
    inline void      SetPreviousFrame(pVMFrame);
    inline void      ClearPreviousFrame();
           bool      HasPreviousFrame() const;
    inline bool      IsBootstrapFrame() const;
    inline pVMFrame  GetContext() const;
    inline void      SetContext(pVMFrame);
           bool      HasContext() const;
           pVMFrame  GetContextLevel(long) const;
           pVMFrame  GetOuterContext() const;
           pVMMethod GetMethod() const;
           void      SetMethod(pVMMethod);
           pVMObject Pop();
           void      Push(pVMObject);
           void      ResetStackPointer();
           long      GetBytecodeIndex() const;
           void      SetBytecodeIndex(long);
           pVMObject GetStackElement(long) const;
           void      SetStackElement(long, pVMObject);
           pVMObject GetLocal(long, long) const;
           void      SetLocal(long, long, pVMObject);
           pVMObject GetArgument(long, long) const;
           void      SetArgument(long, long, pVMObject);
           void      PrintStackTrace() const;
           long      ArgumentStackIndex(long index) const;
           void      CopyArgumentsFrom(pVMFrame frame);

    virtual void MarkReferences();

           void       PrintStack() const;
    inline pVMInteger GetStackPointer() const;
           long       RemainingStackSize() const;
private:
    pVMFrame previousFrame;
    pVMFrame context;
    pVMMethod method;
    pVMInteger stackPointer;
    pVMInteger bytecodeIndex;
    pVMInteger localOffset;

    static const long VMFrameNumberOfFields;
};

bool VMFrame::IsBootstrapFrame() const {
    return !HasPreviousFrame();
}

pVMFrame VMFrame::GetContext() const {
    return this->context;
}

void VMFrame::SetContext(pVMFrame frm) {
    this->context = frm;
}

pVMInteger VMFrame::GetStackPointer() const {
    return stackPointer;
}

pVMFrame VMFrame::GetPreviousFrame() const {
    return this->previousFrame;
}

void VMFrame::SetPreviousFrame(pVMFrame frm) {
    this->previousFrame = frm;
}

void VMFrame::ClearPreviousFrame() {
    this->previousFrame = static_cast<pVMFrame>(nilObject);
}

#endif
