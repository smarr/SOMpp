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

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMObject.h"

const size_t VMArray::VMArrayNumberOfFields = 0;

VMArray::VMArray(size_t arraySize, size_t additionalBytes) :
        VMObject(arraySize + 0 /* VMArray is not allowed to have any fields itself */, additionalBytes + sizeof(VMArray)) {
    assert(VMArrayNumberOfFields == 0);
    nilInitializeFields();
}

vm_oop_t VMArray::GetIndexableField(size_t idx) const {
    if (unlikely(idx > GetNumberOfIndexableFields())) {
        Universe::ErrorExit(("Array index out of bounds: Accessing " +
                             to_string(idx) + ", but array size is only " +
                             to_string(GetNumberOfIndexableFields()) + "\n").c_str());
    }
    return GetField(idx);
}

void VMArray::SetIndexableField(size_t idx, vm_oop_t value) {
    if (unlikely(idx > GetNumberOfIndexableFields())) {
        Universe::ErrorExit(("Array index out of bounds: Accessing " +
                             to_string(idx) + ", but array size is only " +
                             to_string(GetNumberOfIndexableFields()) + "\n").c_str());
    }
    SetField(idx, value);
}

VMArray* VMArray::CopyAndExtendWith(vm_oop_t item) const {
    const size_t fields = GetNumberOfIndexableFields();
    VMArray* result = GetUniverse()->NewArray(fields + 1);
    CopyIndexableFieldsTo(result);
    result->SetIndexableField(fields, item);
    return result;
}

VMArray* VMArray::Clone() const {
    const size_t addSpace = totalObjectSize - sizeof(VMArray);
    auto* clone = new (GetHeap<HEAP_CLS>(), addSpace ALLOC_MATURE) VMArray(*this);
    void* destination  = SHIFTED_PTR(clone, sizeof(VMArray));
    const void* source = SHIFTED_PTR(this, sizeof(VMArray));
    memcpy(destination, source, addSpace);
    return clone;
}

void VMArray::MarkObjectAsInvalid() {
    VMObject::MarkObjectAsInvalid();
    const size_t numIndexableFields = GetNumberOfIndexableFields();
    for (size_t i = 0; i < numIndexableFields; ++i) {
        FIELDS[i] = INVALID_GC_POINTER;
    }
}

void VMArray::CopyIndexableFieldsTo(VMArray* to) const {
    const size_t numIndexableFields = GetNumberOfIndexableFields();
    for (size_t i = 0; i < numIndexableFields; ++i) {
        to->SetIndexableField(i, GetIndexableField(i));
    }
}

StdString VMArray::AsDebugString() const {
    return "Array(" + to_string(GetNumberOfIndexableFields()) + ")";
}
