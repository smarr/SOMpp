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
#define FIELDS ((pVMObject*)&clazz)
#define _BC ((uint8_t*)&FIELDS[this->GetNumberOfFields() + this->GetNumberOfIndexableFields()])

const int VMMethod::VMMethodNumberOfFields = 5; 

VMMethod::VMMethod(int bcCount, int numberOfConstants, int nof) 
                    : VMInvokable(nof + VMMethodNumberOfFields) {
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
    _HEAP->writeBarrier(this, bcLength);
    _HEAP->writeBarrier(this, numberOfLocals);
    _HEAP->writeBarrier(this, maximumNumberOfStackElements);
    _HEAP->writeBarrier(this, numberOfArguments);
    _HEAP->writeBarrier(this, this->numberOfConstants);
    for (int i = 0; i < numberOfConstants ; ++i) {
        this->SetIndexableField(i, nilObject);
    }
}

#ifdef USE_TAGGING
VMMethod* VMMethod::Clone() const {
	VMMethod* clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod), true)
#else
pVMMethod VMMethod::Clone() const {
	pVMMethod clone = new (_HEAP, GetObjectSize() - sizeof(VMMethod), true)
#endif
		VMMethod(*this);
	memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,
				sizeof(VMObject)), GetObjectSize() -
			sizeof(VMObject));
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
	for (int i = 0 ; i < GetNumberOfIndexableFields() ; ++i) {
		if (GetIndexableField(i) != NULL)
			SetIndexableField(i, walk(GetIndexableField(i)));
	}
}


int VMMethod::GetNumberOfLocals() const {
#ifdef USE_TAGGING
    return (int32_t)numberOfLocals; 
#else
    return numberOfLocals->GetEmbeddedInteger(); 
#endif
}


void VMMethod::SetNumberOfLocals(int nol) {
#ifdef USE_TAGGING
    numberOfLocals = nol;
#else
    numberOfLocals = _UNIVERSE->NewInteger(nol);
    _HEAP->writeBarrier(this, numberOfLocals);
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
    _HEAP->writeBarrier(this, maximumNumberOfStackElements);
#endif
}


int VMMethod::GetNumberOfArguments() const {
#ifdef USE_TAGGING
    return (int32_t)numberOfArguments; 
#else
    return numberOfArguments->GetEmbeddedInteger(); 
#endif
}


void VMMethod::SetNumberOfArguments(int noa) {
#ifdef USE_TAGGING
    numberOfArguments = noa;
#else
    numberOfArguments = _UNIVERSE->NewInteger(noa);
    _HEAP->writeBarrier(this, numberOfArguments);
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
    uint8_t bc = _BC[indx+1];
    if (bc >= this->GetNumberOfIndexableFields()) {
        cout << "Error: Constant index out of range" << endl;
        return NULL;
    }
    return this->GetIndexableField(bc);
}

uint8_t& VMMethod::operator[](int indx) const {
	return _BC[indx];
}

uint8_t VMMethod::GetBytecode(int indx) const {
    return _BC[indx];
}


void VMMethod::SetBytecode(int indx, uint8_t val) {
    _BC[indx] = val;
}


//VMArray Methods
pVMArray VMMethod::CopyAndExtendWith(pVMObject item) const {
    size_t fields = this->GetNumberOfIndexableFields();
	pVMArray result = _UNIVERSE->NewArray(fields+1);
    this->CopyIndexableFieldsTo(result);
	result->SetIndexableField(fields, item);
	return result;
}


pVMObject VMMethod::GetIndexableField(int idx) const {
    if (idx > this->GetNumberOfIndexableFields()-1 || idx < 0) {
        cout << "Array index out of bounds: Accessing " << idx
             << ", but only " << GetNumberOfIndexableFields()-1
             << " entries are available\n";
        _UNIVERSE->ErrorExit("Array index out of bounds exception");
    }
	return GetField(this->GetNumberOfFields()+idx);
}


void VMMethod::CopyIndexableFieldsTo(pVMArray to) const {
	for (int i = 0; i < this->GetNumberOfIndexableFields(); ++i) {
        to->SetIndexableField(i, GetIndexableField(i));
	}
}

void VMMethod::SetIndexableField(int idx, pVMObject item) {
    if (idx > this->GetNumberOfIndexableFields()-1 || idx < 0) {
        cout << "Array index out of bounds: Accessing " << idx 
             << ", but there is only space for " 
             << this->GetNumberOfIndexableFields() 
             << " entries available\n";
        _UNIVERSE->ErrorExit("Array index out of bounds exception");
    }
	SetField(this->GetNumberOfFields()+idx, item);
}


int VMMethod::GetNumberOfIndexableFields() const {
    //cannot be done using GetAdditionalSpaceConsumption,
    //as bytecodes need space, too, and there might be padding
#ifdef USE_TAGGING
    return (int32_t)this->numberOfConstants;
#else
    return this->numberOfConstants->GetEmbeddedInteger();
#endif
}

