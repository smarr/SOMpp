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
    _HEAP->StartUninterruptableAllocation();
	
    for (int i = 0; i < size ; ++i) {
        (*this)[i] = nilObject;
    }
    _HEAP->EndUninterruptableAllocation();

}


pVMArray VMArray::CopyAndExtendWith(pVMObject item) const {
    size_t fields = GetNumberOfIndexableFields();
	pVMArray result = _UNIVERSE->NewArray(fields+1);
    this->CopyIndexableFieldsTo(result);
	(*result)[fields] = item;
	return result;
}


pVMObject& VMArray::operator[](int idx) const {
    if (idx > GetNumberOfIndexableFields()-1 || idx < 0) {
        cout << "Array index out of bounds: Accessing " << idx 
             << ", but array size is only " << GetNumberOfIndexableFields()-1 
             << endl;
        _UNIVERSE->ErrorExit("Array index out of bounds");
    }
    return theEntries(idx);
}


void VMArray::CopyIndexableFieldsTo(pVMArray to) const {
	for (int i = 0; i < this->GetNumberOfIndexableFields(); ++i) {
        (*to)[i] = (*this)[i];
	}
	
}

int VMArray::GetNumberOfIndexableFields() const {
    return this->GetAdditionalSpaceConsumption() / sizeof(pVMObject);
}

void VMArray::MarkReferences() {
    if (gcfield) return;
    VMObject::MarkReferences();
	for (int i = 0 ; i < GetNumberOfIndexableFields() ; ++i) {
		if (theEntries(i) != NULL)
			theEntries(i)->MarkReferences();
	}
    
}
