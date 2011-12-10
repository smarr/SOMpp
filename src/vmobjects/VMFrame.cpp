#include "VMFrame.h"
#include "VMMethod.h"
#include "VMClass.h"

const int VMFrame::VMFrameNumberOfFields = 8; 

VMFrame::VMFrame(pVMMethod method)
  : VMObject(method->GetMaximumNumberOfStackElements()
             + method->GetNumberOfArguments()
             + method->GetNumberOfLocals()
             + VMFrameNumberOfFields),
    previousFrame((pVMFrame)nilObject),
    context((pVMFrame)nilObject)
{
    bytecodeIndex=0;
    SetMethod(method);
    int32_t noFrameFields = method->GetNumberOfArguments()
        + method->GetNumberOfLocals()
        + method->GetMaximumNumberOfStackElements();
    for (int i=0; i < noFrameFields; i++)
      arguments[i] = nilObject;
    //for (int i=0; i < method->GetNumberOfArguments(); i++)
    //  if (GetArgument(i,0) != NULL)
    //    SetArgument(i, 0, nilObject);
    //for (int i=0; i < method->GetNumberOfLocals(); i++)
    //  if (GetLocal(i,0) != NULL)
    //    SetLocal(i, 0, nilObject);
    //for (int i=0; i < method->GetMaximumNumberOfStackElements(); i++)
    //  SetStackElement(i, nilObject);
}

void VMFrame::SetMethod(pVMMethod m) {
  method = m;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, m);
#endif
  arguments = ((pVMObject*)&stackElements)+1;
  locals = arguments + m->GetNumberOfArguments();
  stackElements = locals + m->GetNumberOfLocals();
}

void      VMFrame::PrintStackTrace() const {
    //TODO
}

pVMFrame VMFrame::GetOuterContext() {
    pVMFrame current = this;
    while (current->context != nilObject)
        current = current->GetContext();
    return current;
}

pVMObject VMFrame::GetArgument(int index, int contextLevel) {
    pVMFrame context = this;
    while (contextLevel > 0) {
      context = context->GetContext();
      contextLevel--;
    }
    return context->arguments[index];
}

pVMObject VMFrame::GetLocal(int index, int contextLevel) {
    pVMFrame context = this;
    while (contextLevel > 0) {
      context = context->GetContext();
      contextLevel--;
    }
    return context->locals[index];
}

void      VMFrame::SetArgument(int index, int contextLevel, pVMObject value) {
    pVMFrame context = this;
    while (contextLevel > 0) {
      context = context->GetContext();
      contextLevel--;
    }
    context->arguments[index] = value;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, value);
#endif
}

pVMObject VMFrame::Pop() {
#ifdef USE_TAGGING
    int32_t sp = stackPointer;
    stackPointer = sp - 1;
#else
    int32_t sp = stackPointer->GetEmbeddedInteger();
    stackPointer = _UNIVERSE->NewInteger(sp-1);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, stackPointer);
#endif
    return stackElements[sp];
}

void      VMFrame::Push(pVMObject obj) {
#ifdef USE_TAGGING
    int32_t sp = (int32_t)stackPointer + 1;
    stackPointer = sp;
#else
    int32_t sp = stackPointer->GetEmbeddedInteger() + 1;
    stackPointer = _UNIVERSE->NewInteger(sp);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, stackPointer);
#endif
    stackElements[sp] = obj;
}

pVMObject VMFrame::GetStackElement(int index) const {
#ifdef USE_TAGGING
    int sp = (int32_t)this->stackPointer;
#else
    int sp = this->stackPointer->GetEmbeddedInteger();
#endif
    return stackElements[sp-index];
}

void      VMFrame::ResetStackPointer() {
#ifdef USE_TAGGING
    this->stackPointer = -1;
#else
    stackPointer = _UNIVERSE->NewInteger(-1);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, stackPointer);
#endif
}

int VMFrame::RemainingStackSize() const {
  return method->GetMaximumNumberOfStackElements() - stackPointer->GetEmbeddedInteger();
}

void      VMFrame::CopyArgumentsFrom(pVMFrame frame) {
  // copy arguments from frame:
  // - arguments are at the top of the stack of frame.
  // - copy them into the argument area of the current frame
  pVMMethod meth = this->GetMethod();
  int num_args = meth->GetNumberOfArguments();
  for(int i=0; i < num_args; ++i) {
    pVMObject stackElem = frame->GetStackElement(num_args - 1 - i);
    arguments[i] = stackElem;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, stackElem);
#endif

  }
}

void VMFrame::SetLocal(int index, int contextLevel, pVMObject value) {
    pVMFrame context = this;
    while (contextLevel > 0) {
      context = context->GetContext();
      contextLevel--;
    }
    context->locals[index] = value;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, value);
#endif
}

void      VMFrame::SetStackElement(int index, pVMObject obj) {
#ifdef USE_TAGGING
    int sp = (int32_t)this->stackPointer;
#else
    int sp = this->stackPointer->GetEmbeddedInteger();
#endif
	stackElements[sp-index] = obj;
}

#ifdef USE_TAGGING
VMFrame* VMFrame::Clone() const {
#else
pVMFrame VMFrame::Clone() const {
#endif
	int32_t addSpace = objectSize - sizeof(VMFrame);
#ifdef USE_TAGGING
#if GC_TYPE==GENERATIONAL
	VMFrame* clone = new (_HEAP, addSpace, true) VMFrame(*this);
#else
	VMFrame* clone = new (_HEAP, addSpace) VMFrame(*this);
#endif
#else
#if GC_TYPE==GENERATIONAL
	pVMFrame clone = new (_HEAP, addSpace, true) VMFrame(*this);
#else
	pVMFrame clone = new (_HEAP, addSpace) VMFrame(*this);
#endif
#endif
	void* destination = SHIFTED_PTR(clone, sizeof(VMFrame));
	const void* source = SHIFTED_PTR(this, sizeof(VMFrame));
	size_t noBytes = GetObjectSize() - sizeof(VMFrame);
	memcpy(destination, source, noBytes);
  //SetMethod will recalculate array offsets
  clone->SetMethod(method);
	return clone;
}

#ifdef USE_TAGGING
void VMFrame::WalkObjects(AbstractVMObject* (*walk)(AbstractVMObject*)) {
#else
void VMFrame::WalkObjects(pVMObject (*walk)(pVMObject)) {
#endif
  clazz = (VMClass*)walk(clazz);
  previousFrame = (VMFrame*)walk(previousFrame);
  context = (VMFrame*)walk(context);
  method = (VMMethod*)walk(method);
  stackPointer = (VMInteger*)walk(stackPointer);

  for (int i=0; i < method->GetNumberOfArguments(); i++)
    if (GetArgument(i,0) != NULL)
      SetArgument(i, 0, walk(GetArgument(i,0)));
  for (int i=0; i < method->GetNumberOfLocals(); i++)
    if (GetLocal(i,0) != NULL)
      SetLocal(i, 0, walk(GetLocal(i, 0)));
  for (int i=0; i < method->GetMaximumNumberOfStackElements(); i++)
    if (stackElements[i] != NULL) {
      stackElements[i] = walk(stackElements[i]);
#if GC_TYPE==GENETATIONAL
      _HEAP->writeBarrier(this, stackElements[i]);
#endif
    }
}

//when doesNotUnderstand or UnknownGlobal is sent, additional stack slots might
//be necessary, as these cases are not taken into account when the stack
//depth is calculated. In that case this method is called.
pVMFrame VMFrame::EmergencyFrameFrom( pVMFrame from, int extraLength ) {
  int length = from->method->GetNumberOfArguments() 
      + from->method->GetNumberOfLocals()
      + from->method->GetMaximumNumberOfStackElements()
      + extraLength;
  int additionalBytes = length * sizeof(pVMObject);
  pVMFrame result = new (_HEAP, additionalBytes) VMFrame(from->method);

  result->SetClass(from->GetClass());
  //copy arguments, locals and the stack
  for (int i=0; i < from->method->GetNumberOfArguments(); i++)
    result->SetArgument(i, 0, from->GetArgument(i,0));
  for (int i=0; i < from->method->GetNumberOfLocals(); i++)
    result->SetLocal(i, 0, from->GetLocal(i, 0));
  for (int i=0; i < from->method->GetMaximumNumberOfStackElements(); i++)
    result->SetStackElement(i, from->GetStackElement(i));

  //set Frame members
  result->SetPreviousFrame(from->GetPreviousFrame());
  result->SetMethod(from->GetMethod());
  result->SetContext(from->GetContext());
  
  result->stackPointer = _UNIVERSE->NewInteger(from->GetStackPointer());

  result->bytecodeIndex = from->bytecodeIndex;

#if GC_TYPE==GENETATIONAL
  _HEAP->writeBarrier(result, result->stackPointer);
#endif

  return result;
}
