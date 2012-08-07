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


#include "VMMethod.h"
#include "VMFrame.h"
#include "VMClass.h"
#include "VMSymbol.h"
#include "VMArray.h"
#include "VMObject.h"
#include "VMInteger.h"
#include "Signature.h"

#include "../vm/Universe.h"

#include "../compiler/MethodGenerationContext.h"

//this method's bytecodes
//#define FIELDS ((pVMObject*)&clazz)
//#define _BC ((uint8_t*)&FIELDS[this->GetNumberOfFields() + this->GetNumberOfIndexableFields()])

#ifdef UNSAFE_FRAME_OPTIMIZATION
const long VMMethod::VMMethodNumberOfFields = 8;
#else
const long VMMethod::VMMethodNumberOfFields = 7;
#endif

VMMethod::VMMethod(long bcCount, long numberOfConstants, long nof)
  : VMInvokable(nof + VMMethodNumberOfFields) {
#ifdef UNSAFE_FRAME_OPTIMIZATION
    cachedFrame = NULL;
#endif
#ifdef USE_TAGGING
    bcLength = TAG_INTEGER(bcCount);
    numberOfLocals = TAG_INTEGER(0);
    maximumNumberOfStackElements = TAG_INTEGER(0);
    numberOfArguments = TAG_INTEGER(0);
    this->numberOfConstants = TAG_INTEGER(numberOfConstants);
#else
    bcLength = _UNIVERSE->NewInteger( bcCount );
    numberOfLocals = _UNIVERSE->NewInteger(0);
    maximumNumberOfStackElements = _UNIVERSE->NewInteger(0);
    numberOfArguments = _UNIVERSE->NewInteger(0);
    this->numberOfConstants = _UNIVERSE->NewInteger(numberOfConstants);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, bcLength);
    _HEAP->writeBarrier(this, numberOfLocals);
    _HEAP->writeBarrier(this, maximumNumberOfStackElements);
    _HEAP->writeBarrier(this, numberOfArguments);
    _HEAP->writeBarrier(this, this->numberOfConstants);
#endif
    indexableFields = (pVMObject*)(&indexableFields + 2);
    for (long i = 0; i < numberOfConstants ; ++i) {
      indexableFields[i] = nilObject;
      //no need for write barrier (nilObject is found anyway)
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());
  }



pVMMethod VMMethod::Clone() const {
#if GC_TYPE==GENERATIONAL
	pVMMethod clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod), true)
#else
	pVMMethod clone = new (_HEAP, objectSize - sizeof(VMMethod))
#endif
		VMMethod(*this);
	memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
				sizeof(VMObject)), GetObjectSize() -
			sizeof(VMObject));
  clone->indexableFields = (pVMObject*)(&(clone->indexableFields) + 2);
  clone->bytecodes = (uint8_t*)(&(clone->indexableFields) + 2 + GetNumberOfIndexableFields());
	return clone;
}

void VMMethod::SetSignature(pVMSymbol sig) { 
    VMInvokable::SetSignature(sig);
    SetNumberOfArguments(Signature::GetNumberOfArguments(signature));
}


void VMMethod::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
  VMInvokable::WalkObjects(walk);

  numberOfLocals = static_cast<VMInteger*>(walk(numberOfLocals));
  maximumNumberOfStackElements = static_cast<VMInteger*>(walk(maximumNumberOfStackElements));
  bcLength = static_cast<VMInteger*>(walk(bcLength));
  numberOfArguments = static_cast<VMInteger*>(walk(numberOfArguments));
  numberOfConstants = static_cast<VMInteger*>(walk(numberOfConstants));
#ifdef UNSAFE_FRAME_OPTIMIZATION
  if (cachedFrame != NULL)
    cachedFrame = static_cast<VMFrame*>(walk(cachedFrame));
#endif
    
	for (long i = 0 ; i < GetNumberOfIndexableFields() ; ++i) {
		if (GetIndexableField(i) != NULL)
			SetIndexableField(i, walk((VMOBJECT_PTR)GetIndexableField(i)));
	}
}

#ifdef UNSAFE_FRAME_OPTIMIZATION
pVMFrame VMMethod::GetCachedFrame() const {
  return cachedFrame;
}

void VMMethod::SetCachedFrame(pVMFrame frame) {
  cachedFrame = frame;
  if (frame != NULL) {
    frame->SetContext(NULL);
    frame->SetBytecodeIndex(0);
    frame->ResetStackPointer();
#if GC_TYPE == GENERATIONAL
    _HEAP->writeBarrier(this, cachedFrame);
#endif
  }
}
#endif





void VMMethod::SetNumberOfLocals(long nol) {
#ifdef USE_TAGGING
    numberOfLocals = TAG_INTEGER(nol);
#else
    numberOfLocals = _UNIVERSE->NewInteger(nol);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, numberOfLocals);
#endif
}


long VMMethod::GetMaximumNumberOfStackElements() const {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(maximumNumberOfStackElements);
#else
    return maximumNumberOfStackElements->GetEmbeddedInteger(); 
#endif
}


void VMMethod::SetMaximumNumberOfStackElements(long stel) {
#ifdef USE_TAGGING
    maximumNumberOfStackElements = TAG_INTEGER(stel);
#else
    maximumNumberOfStackElements = _UNIVERSE->NewInteger(stel);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, maximumNumberOfStackElements);
#endif
}

void VMMethod::SetNumberOfArguments(long noa) {
#ifdef USE_TAGGING
    numberOfArguments = TAG_INTEGER(noa);
#else
    numberOfArguments = _UNIVERSE->NewInteger(noa);
#endif
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, numberOfArguments);
#endif
}


long VMMethod::GetNumberOfBytecodes() const {
#ifdef USE_TAGGING
    return UNTAG_INTEGER(bcLength);
#else
    return bcLength->GetEmbeddedInteger();
#endif
}


void VMMethod::operator()(pVMFrame frame) {
    pVMFrame frm = _UNIVERSE->GetInterpreter()->PushNewFrame(this);
    frm->CopyArgumentsFrom(frame);
}


void VMMethod::SetHolderAll(pVMClass hld) {
    for (long i = 0; i < this->GetNumberOfIndexableFields(); ++i) {
        pVMObject o = GetIndexableField(i);
        if (!IS_TAGGED(o)) {
          pVMInvokable vmi = dynamic_cast<pVMInvokable>(GET_POINTER(o));
          if ( vmi != NULL)  {
            vmi->SetHolder(hld);
          }
        }
    }
}


pVMObject VMMethod::GetConstant(long indx) const {
    if (bytecodes[indx+1] >= this->GetNumberOfIndexableFields()) {
        cout << "Error: Constant index out of range" << endl;
        return NULL;
    }
    return this->GetIndexableField(bytecodes[indx+1]);
}

uint8_t& VMMethod::operator[](long indx) const {
	return bytecodes[indx];
}





