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


class VMFrame : public VMObject {
public:
    static pVMFrame EmergencyFrameFrom(pVMFrame from, long extraLength);

    VMFrame(long size, long nof = 0);
    
    inline pVMFrame   GetPreviousFrame() const;
    inline void       SetPreviousFrame(pVMObject);
    inline void       ClearPreviousFrame();
    inline bool       HasPreviousFrame() const;
    inline bool       IsBootstrapFrame() const;
    inline pVMFrame   GetContext() const;
    inline void       SetContext(pVMFrame);
    inline bool       HasContext() const;
    pVMFrame   GetContextLevel(long);
    pVMFrame   GetOuterContext();
    inline pVMMethod  GetMethod() const;
    void       SetMethod(pVMMethod);
    pVMObject  Pop();
    void       Push(pVMObject);
    void       ResetStackPointer();
    inline long        GetBytecodeIndex() const;
    inline void       SetBytecodeIndex(long);
    pVMObject  GetStackElement(long) const;
    void       SetStackElement(long, pVMObject);
    pVMObject  GetLocal(long, long);
    void       SetLocal(long, long, pVMObject);
    pVMObject GetArgument(long, long);
    void       SetArgument(long, long, pVMObject);
    void       PrintStackTrace() const;
    long        ArgumentStackIndex(long index) const;
    void       CopyArgumentsFrom(pVMFrame frame);
	  inline virtual pVMObject GetField(long index) const;
#ifdef USE_TAGGING
    virtual VMFrame*   Clone() const;
		virtual void WalkObjects(AbstractVMObject* (AbstractVMObject*));
#else
		virtual void WalkObjects(pVMObject (pVMObject));
    virtual pVMFrame   Clone() const;
#endif
    
    void       PrintStack() const;
    inline void* GetStackPointer() const;
    long        RemainingStackSize() const;
private:
    pVMFrame   previousFrame;
    pVMFrame   context;
    pVMMethod  method;
    long    bytecodeIndex;
    pVMObject* arguments;
    pVMObject* locals;
    pVMObject* stack_ptr;

    static const long VMFrameNumberOfFields;
};

pVMObject VMFrame::GetField(long index) const {
  if (index==4)
    return _UNIVERSE->NewInteger(bytecodeIndex);
  return VMObject::GetField(index);
}

bool     VMFrame::HasContext() const {
    return this->context !=  NULL; 
}

bool     VMFrame::HasPreviousFrame() const {
    return this->previousFrame != NULL;
}

long       VMFrame::GetBytecodeIndex() const {
  return bytecodeIndex;
}

void      VMFrame::SetBytecodeIndex(long index) {
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

void* VMFrame::GetStackPointer() const {
  return stack_ptr;
}



pVMFrame VMFrame::GetPreviousFrame() const {
    return this->previousFrame;
}


void     VMFrame::SetPreviousFrame(pVMObject frm) {
    this->previousFrame = static_cast<pVMFrame>(frm);
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, GET_POINTER(frm));
#endif
}

void     VMFrame::ClearPreviousFrame() {
    this->previousFrame = NULL;
}

pVMMethod VMFrame::GetMethod() const {
    return this->method;
}
#endif
