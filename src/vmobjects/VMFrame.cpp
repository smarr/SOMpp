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


#include "VMFrame.h"
#include "VMMethod.h"
#include "VMObject.h"
#include "VMInteger.h"
#include "VMClass.h"
#include "VMSymbol.h"

#include "../vm/Universe.h"

//when doesNotUnderstand or UnknownGlobal is sent, additional stack slots might
//be necessary, as these cases are not taken into account when the stack
//depth is calculated. In that case this method is called.
pVMFrame VMFrame::EmergencyFrameFrom( pVMFrame from, int extraLength ) {
  int length = from->GetNumberOfIndexableFields() + extraLength;
  int additionalBytes = length * sizeof(pVMObject);
  pVMFrame result = new (_HEAP, additionalBytes) VMFrame(length);

  result->SetClass(from->GetClass());
  //copy arguments, locals and the stack
  from->CopyIndexableFieldsTo(result);

  //set Frame members
  result->SetPreviousFrame(from->GetPreviousFrame());
  result->SetMethod(from->GetMethod());
  result->SetContext(from->GetContext());
  
  result->stackPointer = _UNIVERSE->NewInteger(from->GetStackPointer());

#ifdef USE_TAGGING
  result->bytecodeIndex = (int32_t)(from->bytecodeIndex);
#else
  result->bytecodeIndex = _UNIVERSE->NewInteger(from->bytecodeIndex->GetEmbeddedInteger());
#endif
  
#ifdef USE_TAGGING
  result->localOffset = (int32_t)(from->localOffset);
#else
  result->localOffset = _UNIVERSE->NewInteger(from->localOffset->GetEmbeddedInteger());
#endif

#if GC_TYPE==GENETATIONAL
  _HEAP->writeBarrier(result, result->stackPointer);
  _HEAP->writeBarrier(result, result->localOffset);
  _HEAP->writeBarrier(result, result->bytecodeIndex);
#endif

  return result;
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
	return clone;
}

const int VMFrame::VMFrameNumberOfFields = 6; 

VMFrame::VMFrame(int size, int nof) :
		VMArray(size, nof + VMFrameNumberOfFields),
		previousFrame((pVMFrame)nilObject), context((pVMFrame)nilObject),
		method((pVMMethod)nilObject) {
#ifdef USE_TAGGING
    this->localOffset = 0;
    this->bytecodeIndex = 0;
    this->stackPointer = 0;
#else
    this->localOffset = _UNIVERSE->NewInteger(0);
    this->bytecodeIndex = _UNIVERSE->NewInteger(0);
    this->stackPointer = _UNIVERSE->NewInteger(0);
#endif
#if GC_TYPE==GENERATIONAL
	_HEAP->writeBarrier(this, localOffset);
	_HEAP->writeBarrier(this, bytecodeIndex);
	_HEAP->writeBarrier(this, stackPointer);
#endif
}


void      VMFrame::SetMethod(pVMMethod method) {
    this->method = method;
#if GC_TYPE==GENERATIONAL
	_HEAP->writeBarrier(this, method);
#endif
}

bool     VMFrame::HasPreviousFrame() const {
    return this->previousFrame != nilObject;
}

bool     VMFrame::HasContext() const {
    return this->context !=  nilObject; 
}


pVMFrame VMFrame::GetContextLevel(int lvl) {
    pVMFrame current = this;
    while (lvl > 0) {
        current = current->GetContext();
        --lvl;
    }
    return current;
}


pVMFrame VMFrame::GetOuterContext() {
    pVMFrame current = this;
    while (current->HasContext()) {
        current = current->GetContext();
    }
    return current;
}



int VMFrame::RemainingStackSize() const {
    // - 1 because the stack pointer points at the top entry,
    // so the next entry would be put at stackPointer+1
    return this->GetNumberOfIndexableFields() -
#ifdef USE_TAGGING
           (int32_t)stackPointer - 1;
#else
           stackPointer->GetEmbeddedInteger() - 1;
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
    return GetIndexableField(sp);
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
    SetIndexableField(sp, obj);
}


void VMFrame::PrintStack() const {
#ifdef USE_TAGGING
    cout << "SP: " << (int32_t)this->stackPointer << endl;
#else
    cout << "SP: " << this->stackPointer->GetEmbeddedInteger() << endl;
#endif
    for (int i = 0; i < this->GetNumberOfIndexableFields()+1; ++i) {
        pVMObject vmo = this->GetIndexableField(i);
        cout << i << ": ";
        if (vmo == NULL) 
            cout << "NULL" << endl;
        if (vmo == nilObject) 
            cout << "NIL_OBJECT" << endl;
        if (vmo->GetClass() == NULL) 
            cout << "VMObject with Class == NULL" << endl;
        if (vmo->GetClass() == nilObject) 
            cout << "VMObject with Class == NIL_OBJECT" << endl;
        else 
            cout << "index: " << i << " object:" 
                 << vmo->GetClass()->GetName()->GetChars() << endl;
    }
}


void      VMFrame::ResetStackPointer() {
    // arguments are stored in front of local variables
    pVMMethod meth = this->GetMethod();
    size_t lo = meth->GetNumberOfArguments();
#ifdef USE_TAGGING
    this->localOffset = lo;
#else
    localOffset = _UNIVERSE->NewInteger(lo);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, localOffset);
#endif
  
    // Set the stack pointer to its initial value thereby clearing the stack
    size_t numLocals = meth->GetNumberOfLocals();
#ifdef USE_TAGGING
    this->stackPointer = lo + numLocals - 1;
#else
    stackPointer = _UNIVERSE->NewInteger(lo + numLocals - 1);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, stackPointer);
#endif
}


int       VMFrame::GetBytecodeIndex() const {
#ifdef USE_TAGGING
    return (int32_t)this->bytecodeIndex;
#else
    return this->bytecodeIndex->GetEmbeddedInteger();
#endif
}


void      VMFrame::SetBytecodeIndex(int index) {
#ifdef USE_TAGGING
  bytecodeIndex = index;
#else
  bytecodeIndex = _UNIVERSE->NewInteger(index);
#endif
#if GC_TYPE==GENERATIONAL
  _HEAP->writeBarrier(this, bytecodeIndex);
#endif
}


pVMObject VMFrame::GetStackElement(int index) const {
#ifdef USE_TAGGING
    int sp = (int32_t)this->stackPointer;
#else
    int sp = this->stackPointer->GetEmbeddedInteger();
#endif
    return GetIndexableField(sp-index);
}


void      VMFrame::SetStackElement(int index, pVMObject obj) {
#ifdef USE_TAGGING
    int sp = (int32_t)this->stackPointer;
#else
    int sp = this->stackPointer->GetEmbeddedInteger();
#endif
	SetIndexableField(sp-index, obj);
}


pVMObject VMFrame::GetLocal(int index, int contextLevel) {
    pVMFrame context = this->GetContextLevel(contextLevel);
#ifdef USE_TAGGING
    int32_t lo = (int32_t)context->localOffset;
#else
    int32_t lo = context->localOffset->GetEmbeddedInteger();
#endif
    return context->GetIndexableField(lo+index);
}


void      VMFrame::SetLocal(int index, int contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
#ifdef USE_TAGGING
    size_t lo = (int32_t)context->localOffset;
#else
    size_t lo = context->localOffset->GetEmbeddedInteger();
#endif
    context->SetIndexableField(lo+index, value);
}



pVMObject VMFrame::GetArgument(int index, int contextLevel) {
    // get the context
    pVMFrame context = this->GetContextLevel(contextLevel);
    return context->GetIndexableField(index);
}


void      VMFrame::SetArgument(int index, int contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    context->SetIndexableField(index, value);
}


void      VMFrame::PrintStackTrace() const {
    //TODO
}

int       VMFrame::ArgumentStackIndex(int index) const {
    pVMMethod meth = this->GetMethod();
    return meth->GetNumberOfArguments() - index - 1;
}


void      VMFrame::CopyArgumentsFrom(pVMFrame frame) {
    // copy arguments from frame:
    // - arguments are at the top of the stack of frame.
    // - copy them into the argument area of the current frame
    pVMMethod meth = this->GetMethod();
    int num_args = meth->GetNumberOfArguments();
    for(int i=0; i < num_args; ++i) {
        pVMObject stackElem = frame->GetStackElement(num_args - 1 - i);
        SetIndexableField(i, stackElem);
    }
}

