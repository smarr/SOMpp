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


class VMFrame : public VMObject {
public:
    static pVMFrame EmergencyFrameFrom(pVMFrame from, int extraLength);

    VMFrame(int size, int nof = 0);
    
    inline pVMFrame   GetPreviousFrame() const;
    inline void       SetPreviousFrame(pVMObject);
    inline void       ClearPreviousFrame();
    inline bool       HasPreviousFrame() const;
    inline bool       IsBootstrapFrame() const;
    inline pVMFrame   GetContext() const;
    inline void       SetContext(pVMFrame);
    inline bool       HasContext() const;
    pVMFrame   GetContextLevel(int);
    pVMFrame   GetOuterContext();
    inline pVMMethod  GetMethod() const;
    void       SetMethod(pVMMethod);
    pVMObject  Pop();
    void       Push(pVMObject);
    void       ResetStackPointer();
    inline int        GetBytecodeIndex() const;
    inline void       SetBytecodeIndex(int);
    pVMObject  GetStackElement(int) const;
    void       SetStackElement(int, pVMObject);
    pVMObject  GetLocal(int, int);
    void       SetLocal(int, int, pVMObject);
    pVMObject GetArgument(int, int);
    void       SetArgument(int, int, pVMObject);
    void       PrintStackTrace() const;
    int        ArgumentStackIndex(int index) const;
    void       CopyArgumentsFrom(pVMFrame frame);
	  inline virtual pVMObject GetField(int index) const;
#ifdef USE_TAGGING
    virtual VMFrame*   Clone() const;
		virtual void WalkObjects(AbstractVMObject* (AbstractVMObject*));
#else
		virtual void WalkObjects(pVMObject (pVMObject));
    virtual pVMFrame   Clone() const;
#endif
    
    void       PrintStack() const;
    inline int32_t GetStackPointer() const;
    int        RemainingStackSize() const;
private:
    pVMFrame   previousFrame;
    pVMFrame   context;
    pVMMethod  method;
    int32_t    bytecodeIndex;
    pVMObject* arguments;
    pVMObject* locals;
    pVMObject* stack_ptr;

    static const int VMFrameNumberOfFields;
};

pVMObject VMFrame::GetField(int32_t index) const {
  if (index==4)
    return _UNIVERSE->NewInteger(bytecodeIndex);
  return VMObject::GetField(index);
}

bool     VMFrame::HasContext() const {
    return this->context !=  nilObject; 
}

bool     VMFrame::HasPreviousFrame() const {
    return this->previousFrame != nilObject;
}

int       VMFrame::GetBytecodeIndex() const {
  return bytecodeIndex;
}

void      VMFrame::SetBytecodeIndex(int index) {
  bytecodeIndex = index;
}

bool     VMFrame::IsBootstrapFrame() const {
    return !HasPreviousFrame();
}

pVMFrame VMFrame::GetContext() const {
    return this->context;
}

void     VMFrame::SetContext(pVMFrame frm) {
    this->context = frm;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, frm);
#endif
}

int32_t VMFrame::GetStackPointer() const {
  return (int32_t)stack_ptr;
}



pVMFrame VMFrame::GetPreviousFrame() const {
    return (pVMFrame) this->previousFrame;
}


void     VMFrame::SetPreviousFrame(pVMObject frm) {
    this->previousFrame = (pVMFrame)frm;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, frm);
#endif
}

void     VMFrame::ClearPreviousFrame() {
    this->previousFrame = (pVMFrame)nilObject;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, nilObject);
#endif
}

pVMMethod VMFrame::GetMethod() const {
    return this->method;
}
#endif
