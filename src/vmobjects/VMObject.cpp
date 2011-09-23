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


#include "VMObject.h"
#include "VMClass.h"
#include "VMSymbol.h"
#include "VMFrame.h"
#include "VMInvokable.h"

#define FIELDS ((pVMObject*)&clazz)

//clazz is the only field of VMObject so
const int VMObject::VMObjectNumberOfFields = 1;

VMObject::VMObject( int numberOfFields ) {
    //this line would be needed if the VMObject** is used instead of the macro:
    //FIELDS = (pVMObject*)&clazz; 
    this->SetNumberOfFields(numberOfFields + VMObjectNumberOfFields);
    gcfield = 0; 
	hash = (int32_t)this;
    //Object size is set by the heap
}

#ifdef USE_TAGGING
VMObject* VMObject::Clone() const {
#else
pVMObject VMObject::Clone() const {
#endif
	VMObject* clone = new (_HEAP, objectSize - sizeof(VMObject), true) VMObject(*this);
	memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
			SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() -
			sizeof(VMObject));
	clone->hash = (int32_t)&clone;
	return clone;
}

void VMObject::SetNumberOfFields(int nof) {
    this->numberOfFields = nof;

    for (int i = 0; i < nof ; ++i) {
        this->SetField(i, nilObject);
    }
}


void VMObject::SetClass(pVMClass cl) {
	clazz = cl;
	_HEAP->writeBarrier(this, cl);
}

pVMSymbol VMObject::GetFieldName(int index) const {
    return this->clazz->GetInstanceFieldName(index);
}


void VMObject::Assert(bool value) const {
    _UNIVERSE->Assert(value);
}


pVMObject VMObject::GetField(int index) const {
    return FIELDS[index]; 
}


void VMObject::SetField(int index, pVMObject value) {
     FIELDS[index] = value;
	 _HEAP->writeBarrier(this, value);
}

//returns the Object's additional memory used (e.g. for Array fields)
int VMObject::GetAdditionalSpaceConsumption() const
{
    //The VM*-Object's additional memory used needs to be calculated.
    //It's      the total object size   MINUS   sizeof(VMObject) for basic
    //VMObject  MINUS   the number of fields times sizeof(pVMObject)
    return (objectSize - (sizeof(VMObject) + 
                          sizeof(pVMObject) * (this->GetNumberOfFields() - 1)));
}

#ifdef USE_TAGGING
void VMObject::WalkObjects(AbstractVMObject* (*walk)(AbstractVMObject*)) {
#else
void VMObject::WalkObjects(pVMObject (*walk)(pVMObject)) {
#endif
    for( int i = 0; i < this->GetNumberOfFields(); ++i) {
		SetField(i, walk(GetField(i)));
    }

}
