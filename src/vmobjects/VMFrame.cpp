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
  pVMMethod method = from->GetMethod();
  int length = method->GetNumberOfArguments()
      + method->GetNumberOfLocals()
      + method->GetMaximumNumberOfStackElements()
      + extraLength;

  int additionalBytes = length * sizeof(pVMObject);
  pVMFrame result = new (_HEAP, additionalBytes) VMFrame(length);

  result->SetClass(from->GetClass());

  //set Frame members
  result->SetPreviousFrame(from->GetPreviousFrame());
  result->SetMethod(method);
  result->SetContext(from->GetContext());
#ifdef USE_TAGGING
  result->stack_ptr = (pVMObject*)SHIFTED_PTR(result.GetPointer(), (int32_t)from->stack_ptr - (int32_t)from.GetPointer());
#else
  result->stack_ptr = (pVMObject*)SHIFTED_PTR(result, (int32_t)from->stack_ptr - (int32_t)from);
#endif
  result->bytecodeIndex = from->bytecodeIndex;
//result->arguments is set in VMFrame constructor
  result->locals = result->arguments + result->method->GetNumberOfArguments();

  //all other fields are indexable via arguments
  // --> until end of Frame
#ifdef USE_TAGGING
  pVMObject* from_end = (pVMObject*) SHIFTED_PTR(from.GetPointer(), from->GetObjectSize());
  pVMObject* result_end = (pVMObject*) SHIFTED_PTR(result.GetPointer(), result->GetObjectSize());
#else
  pVMObject* from_end = (pVMObject*) SHIFTED_PTR(from, from->GetObjectSize());
  pVMObject* result_end = (pVMObject*) SHIFTED_PTR(result, result->GetObjectSize());
#endif

  int32_t i = 0;
  //copy all fields from other frame
  while (from->arguments + i < from_end) {
      result->arguments[i] = from->arguments[i];
#if GC_TYPE==GENERATIONAL
      _HEAP->writeBarrier(result, from->arguments[i]);
#endif
    i++;
  }
  //initialize others with nilObject
  while (result->arguments + i < result_end) {
    result->arguments[i] = nilObject;
    i++;
  }

#if GC_TYPE==GENERATIONAL
  _HEAP->writeBarrier(result, nilObject); //for extra fields
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
  clone->arguments = (pVMObject*)&(clone->stack_ptr)+1;//field after stack_ptr
  clone->locals = clone->arguments + clone->method->GetNumberOfArguments();
  clone->stack_ptr = (pVMObject*)SHIFTED_PTR(clone, (int32_t)stack_ptr - (int32_t)this);
	return clone;
}

const int VMFrame::VMFrameNumberOfFields = 7; 

VMFrame::VMFrame(int size, int nof) :
		VMObject(nof + VMFrameNumberOfFields),
		previousFrame((pVMFrame)nilObject), context((pVMFrame)nilObject),
		method((pVMMethod)nilObject) {
#ifdef USE_TAGGING
    this->bytecodeIndex = 0;
#else
    this->bytecodeIndex = 0;
#endif
  arguments = (pVMObject*)&(stack_ptr)+1;
  locals = arguments;
  stack_ptr = locals;

  //initilize all other fields
  // --> until end of Frame
  pVMObject* end = (pVMObject*) SHIFTED_PTR(this, objectSize);
  int32_t i = 0;
  while (arguments + i < end) {
      arguments[i] = nilObject;
    i++;
  }
#if GC_TYPE==GENERATIONAL
	_HEAP->writeBarrier(this, nilObject);
#endif

}


void      VMFrame::SetMethod(pVMMethod method) {
    this->method = method;
#if GC_TYPE==GENERATIONAL
	_HEAP->writeBarrier(this, method);
#endif
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


#ifdef USE_TAGGING
void VMFrame::WalkObjects(AbstractVMObject* (*walk)(AbstractVMObject*)) {
#else
void VMFrame::WalkObjects(pVMObject (*walk)(pVMObject)) {
#endif
  clazz = (VMClass*)walk(clazz);
  previousFrame = (VMFrame*)walk(previousFrame);
  context = (VMFrame*)walk(context);
  method = (VMMethod*)walk(method);

  //all other fields are indexable via arguments array
  // --> until end of Frame
  pVMObject* end = (pVMObject*) SHIFTED_PTR(this, objectSize);
  int32_t i = 0;
  while (arguments + i < end) {
    if (arguments[i] != NULL)
      arguments[i] = walk(arguments[i]);
    i++;
  }
}



int VMFrame::RemainingStackSize() const {
    // - 1 because the stack pointer points at the top entry,
    // so the next entry would be put at stackPointer+1
    int32_t size = ((int32_t)this+objectSize - int32_t(stack_ptr))/ sizeof(pVMObject);
    return size - 1;
}

pVMObject VMFrame::Pop() {
  return *stack_ptr--;
}


void      VMFrame::Push(pVMObject obj) {
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, obj);
#endif
    *(++stack_ptr) = obj;
}


void VMFrame::PrintStack() const {
#ifdef USE_TAGGING
    cout << "SP: " << (int32_t)this->GetStackPointer() << endl;
#else
    cout << "SP: " << this->GetStackPointer() << endl;
#endif
  //all other fields are indexable via arguments array
  // --> until end of Frame
  pVMObject* end = (pVMObject*) SHIFTED_PTR(this, objectSize);
  int32_t i = 0;
  while (arguments + i < end) {
    pVMObject vmo = arguments[i];
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
    i++;
  }
}


void      VMFrame::ResetStackPointer() {
    // arguments are stored in front of local variables
    pVMMethod meth = this->GetMethod();
    locals = arguments + meth->GetNumberOfArguments();
    // Set the stack pointer to its initial value thereby clearing the stack
    stack_ptr = locals + meth->GetNumberOfLocals() - 1;
}


pVMObject VMFrame::GetStackElement(int index) const {
    return stack_ptr[-index];
}


void      VMFrame::SetStackElement(int index, pVMObject obj) {
  stack_ptr[-index]= obj;
}


pVMObject VMFrame::GetLocal(int index, int contextLevel) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    return context->locals[index];
}


void      VMFrame::SetLocal(int index, int contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    context->locals[index] = value;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(context, value);
#endif
}



pVMObject VMFrame::GetArgument(int index, int contextLevel) {
    // get the context
    pVMFrame context = this->GetContextLevel(contextLevel);
    return context->arguments[index];
}


void      VMFrame::SetArgument(int index, int contextLevel, pVMObject value) {
    pVMFrame context = this->GetContextLevel(contextLevel);
    context->arguments[index] = value;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(context, value);
#endif
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
    int num_args = GetMethod()->GetNumberOfArguments();
    for(int i=0; i < num_args; ++i) {
        pVMObject stackElem = frame->GetStackElement(num_args - 1 - i);
        SetArgument(i, 0, stackElem);
    }
}

