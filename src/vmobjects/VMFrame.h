#pragma once
#ifndef VMFRAME_H_
#define VMFRAME_H_

#include "VMObject.h"
#include "VMIntPointer.h"

class VMFrame : public VMObject {
 public:
  VMFrame(pVMMethod); 
  void       SetLocal(int, int, pVMObject);
  void       SetArgument(int, int, pVMObject);
  void       SetMethod(pVMMethod);
  inline void       SetPreviousFrame(pVMObject);
  void       ResetStackPointer();
  inline bool HasPreviousFrame() const;
  void       PrintStackTrace() const;
  void Push(pVMObject);
  pVMObject  GetStackElement(int) const;
  void       CopyArgumentsFrom(pVMFrame frame);
  inline void SetContext(pVMFrame);
  pVMObject  GetLocal(int, int);
  pVMObject GetArgument(int, int);
  pVMFrame   GetOuterContext();
  inline int32_t GetStackPointer() const;
  inline pVMMethod  GetMethod() const;
  pVMObject  Pop();
  inline void       SetBytecodeIndex(int);
  inline int        GetBytecodeIndex() const;
  inline pVMFrame   GetPreviousFrame() const;
  inline void       ClearPreviousFrame();
  int        RemainingStackSize() const;
  inline pVMFrame   GetContext() const;
  bool       HasContext() const;
  static pVMFrame EmergencyFrameFrom(pVMFrame from, int extraLength);
  void       SetStackElement(int, pVMObject);
#ifdef USE_TAGGING
    virtual VMFrame*   Clone() const;
		virtual void WalkObjects(AbstractVMObject* (AbstractVMObject*));
#else
		virtual void WalkObjects(pVMObject (pVMObject));
    virtual pVMFrame   Clone() const;
#endif
 private:
  int32_t bytecodeIndex;
  pVMInteger stackPointer;
  pVMMethod method;
  pVMFrame previousFrame;
  pVMFrame context;
  pVMObject* arguments;
  pVMObject* locals;
  pVMObject* stackElements;
  static const int VMFrameNumberOfFields;
};

pVMMethod VMFrame::GetMethod() const {
  return method;
}

pVMFrame VMFrame::GetContext() const {
    return this->context;
}

void VMFrame::SetBytecodeIndex(int32_t bcIndex) {
  bytecodeIndex = bcIndex;
}

void     VMFrame::ClearPreviousFrame() {
    this->previousFrame = (pVMFrame)nilObject;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, nilObject);
#endif
}

pVMFrame VMFrame::GetPreviousFrame() const {
  return previousFrame;
}

int32_t VMFrame::GetBytecodeIndex() const {
  return bytecodeIndex;
}

int32_t VMFrame::GetStackPointer() const {
  return stackPointer->GetEmbeddedInteger();
}

bool     VMFrame::HasPreviousFrame() const {
  return this->previousFrame != nilObject;
}

void     VMFrame::SetPreviousFrame(pVMObject frm) {
    this->previousFrame = (pVMFrame)frm;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, frm);
#endif
}

void     VMFrame::SetContext(pVMFrame frm) {
    this->context = frm;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, frm);
#endif
}

#endif
