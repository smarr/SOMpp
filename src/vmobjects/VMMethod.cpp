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
#ifdef USE_TAGGING
#include "VMPointerConverter.h"
#endif

#include "../vm/Universe.h"

#include "../compiler/MethodGenerationContext.h"

//this method's bytecodes
//#define FIELDS ((pVMObject*)&clazz)
//#define _BC ((uint8_t*)&FIELDS[this->GetNumberOfFields() + this->GetNumberOfIndexableFields()])

#ifdef UNSAFE_FRAME_OPTIMIZATION
const int VMMethod::VMMethodNumberOfFields = 8;
#else
const int VMMethod::VMMethodNumberOfFields = 7;
#endif

VMMethod::VMMethod(int bcCount, int numberOfConstants, int nof)
  : VMInvokable(nof + VMMethodNumberOfFields) {
#ifdef UNSAFE_FRAME_OPTIMIZATION
    cachedFrame = NULL;
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, nilObject);
#endif
#endif
#ifdef USE_TAGGING
    bcLength = bcCount ;
    numberOfLocals = 0;
    maximumNumberOfStackElements = 0;
    numberOfArguments = 0;
    this->numberOfConstants = numberOfConstants;
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
    for (int i = 0; i < numberOfConstants ; ++i) {
      indexableFields[i] = nilObject;
      //no need for write barrier (nilObject is found anyway)
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());
  }



#ifdef USE_TAGGING
VMMethod* VMMethod::Clone() const {
#if GC_TYPE==GENERATIONAL
	VMMethod* clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod), true)
#else
	VMMethod* clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod))
#endif
#else
pVMMethod VMMethod::Clone() const {
#if GC_TYPE==GENERATIONAL
	pVMMethod clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod), true)
#else
	pVMMethod clone = new (_HEAP, objectSize - sizeof(VMMethod))
#endif
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


#ifdef USE_TAGGING
void VMMethod::WalkObjects(AbstractVMObject* (*walk)(AbstractVMObject*)) {
#else
void VMMethod::WalkObjects(pVMObject (*walk)(pVMObject)) {
#endif
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
    
	for (int i = 0 ; i < GetNumberOfIndexableFields() ; ++i) {
		if (GetIndexableField(i) != NULL)
			SetIndexableField(i, walk(GetIndexableField(i)));
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
#ifdef USE_TAGGING
    _HEAP->writeBarrier(this, cachedFrame.GetPointer());
#else
    _HEAP->writeBarrier(this, cachedFrame);
#endif
#endif
  }
}
#endif





void VMMethod::SetNumberOfLocals(int nol) {
#ifdef USE_TAGGING
    numberOfLocals = nol;
#else
    numberOfLocals = _UNIVERSE->NewInteger(nol);
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, numberOfLocals);
#endif
#endif
}


int VMMethod::GetMaximumNumberOfStackElements() const {
#ifdef USE_TAGGING
    return (int32_t)maximumNumberOfStackElements;
#else
    return maximumNumberOfStackElements->GetEmbeddedInteger(); 
#endif
}


void VMMethod::SetMaximumNumberOfStackElements(int stel) {
#ifdef USE_TAGGING
    maximumNumberOfStackElements = stel;
#else
    maximumNumberOfStackElements = _UNIVERSE->NewInteger(stel);
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, maximumNumberOfStackElements);
#endif
#endif
}

void VMMethod::SetNumberOfArguments(int noa) {
#ifdef USE_TAGGING
    numberOfArguments = noa;
#else
    numberOfArguments = _UNIVERSE->NewInteger(noa);
#if GC_TYPE==GENERATIONAL
    _HEAP->writeBarrier(this, numberOfArguments);
#endif
#endif
}


int VMMethod::GetNumberOfBytecodes() const {
#ifdef USE_TAGGING
    return (int32_t)bcLength;
#else
    return bcLength->GetEmbeddedInteger();
#endif
}


void VMMethod::operator()(pVMFrame frame) {
    pVMFrame frm = _UNIVERSE->GetInterpreter()->PushNewFrame(this);
    frm->CopyArgumentsFrom(frame);
}


void VMMethod::SetHolderAll(pVMClass hld) {
    for (int i = 0; i < this->GetNumberOfIndexableFields(); ++i) {
        pVMObject o = GetIndexableField(i);
#ifdef USE_TAGGING
        pVMInvokable vmi = DynamicConvert<VMInvokable, VMObject>(o);
#else
        pVMInvokable vmi = dynamic_cast<pVMInvokable>(o);
#endif
        if ( vmi != NULL)  {
            vmi->SetHolder(hld);
        }
    }
}


pVMObject VMMethod::GetConstant(int indx) const {
    if (bytecodes[indx+1] >= this->GetNumberOfIndexableFields()) {
        cout << "Error: Constant index out of range" << endl;
        return NULL;
    }
    return this->GetIndexableField(bytecodes[indx+1]);
}

uint8_t& VMMethod::operator[](int indx) const {
	return bytecodes[indx];
}





