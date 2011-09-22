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

#include "../vm/Universe.h"


#define theEntries(i) FIELDS[this->GetNumberOfFields()+i]

const int VMArray::VMArrayNumberOfFields = 0; 

VMArray::VMArray(int size, int nof) : VMObject(nof + VMArrayNumberOfFields) {
    for (int i = 0; i < size ; ++i) {
        SetIndexableField(i, nilObject);
    }
}
	
pVMObject VMArray::GetIndexableField(int32_t idx) const {
    if (idx > GetNumberOfIndexableFields()-1 || idx < 0) {
        cout << "Array index out of bounds: Accessing " << idx 
             << ", but array size is only " << GetNumberOfIndexableFields()-1 
             << endl;
        _UNIVERSE->ErrorExit("Array index out of bounds");
    }
	return GetField(this->GetNumberOfFields()+idx);
}

void VMArray::SetIndexableField(int32_t idx, pVMObject value) {
    if (idx > GetNumberOfIndexableFields()-1 || idx < 0) {
        cout << "Array index out of bounds: Accessing " << idx 
             << ", but array size is only " << GetNumberOfIndexableFields()-1 
             << endl;
        _UNIVERSE->ErrorExit("Array index out of bounds");
    }
    SetField(this->GetNumberOfFields()+idx, value);
}

pVMArray VMArray::CopyAndExtendWith(pVMObject item) const {
    size_t fields = GetNumberOfIndexableFields();
	pVMArray result = _UNIVERSE->NewArray(fields+1);
    this->CopyIndexableFieldsTo(result);
	result->SetIndexableField(fields, item);
	return result;
}



#ifdef USE_TAGGING
VMArray* VMArray::Clone() const {
#else
pVMArray VMArray::Clone() const {
#endif
	int32_t addSpace = objectSize - sizeof(VMArray);
#ifdef USE_TAGGING
	VMArray* clone = new (_HEAP, addSpace, true) VMArray(*this);
#else
	pVMArray clone = new (_HEAP, addSpace, true) VMArray(*this);
#endif
	void* destination = SHIFTED_PTR(clone, sizeof(VMArray));
	const void* source = SHIFTED_PTR(this, sizeof(VMArray));
	size_t noBytes = GetObjectSize() - sizeof(VMArray);
	memcpy(destination, source, noBytes);
	return clone;
}

void VMArray::CopyIndexableFieldsTo(pVMArray to) const {
	for (int i = 0; i < this->GetNumberOfIndexableFields(); ++i) {
		to->SetIndexableField(i, GetIndexableField(i));
	}	
}

int VMArray::GetNumberOfIndexableFields() const {
    return this->GetAdditionalSpaceConsumption() / sizeof(pVMObject);
}

#ifdef USE_TAGGING
void VMArray::WalkObjects(AbstractVMObject* (*walk)(AbstractVMObject*)) {
#else
void VMArray::WalkObjects(pVMObject (*walk)(pVMObject)) {
#endif
	int32_t noOfFields = GetNumberOfFields();
    for (int32_t i = 0; i < noOfFields; i++)
	    SetField(i, walk(GetField(i)));
    for (int32_t i = 0; i < GetNumberOfIndexableFields(); i++)
	    if (this->GetIndexableField(i) != NULL)
		    SetIndexableField(i, walk(GetIndexableField(i)));
}
