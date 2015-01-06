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
#include "VMClass.h"

#include <vm/Universe.h>

const long VMArray::VMArrayNumberOfFields = 0;

VMArray::VMArray(long size, long nof) :
        VMObject(nof + VMArrayNumberOfFields) {
    // initialize fields with nilObject
    // SetIndexableField is not used to prevent the write barrier to be called
    // too often.
    // Fields start after clazz and other fields (GetNumberOfFields)
    gc_oop_t* arrFields = FIELDS + GetNumberOfFields();
    for (long i = 0; i < size; ++i) {
        arrFields[i] = nilObject;
    }
}

vm_oop_t VMArray::GetIndexableField(long idx) const {
    if (unlikely(idx > GetNumberOfIndexableFields())) {
        GetUniverse()->ErrorExit(("Array index out of bounds: Accessing " +
                                 to_string(idx) + ", but array size is only " +
                                 to_string(GetNumberOfIndexableFields()) + "\n").c_str());
    }
    return GetField(GetNumberOfFields() + idx);
}

void VMArray::SetIndexableField(long idx, vm_oop_t value) {
    if (unlikely(idx > GetNumberOfIndexableFields())) {
        GetUniverse()->ErrorExit(("Array index out of bounds: Accessing " +
                                  to_string(idx) + ", but array size is only " +
                                  to_string(GetNumberOfIndexableFields()) + "\n").c_str());
    }
    SetField(GetNumberOfFields() + idx, value);
}

VMArray* VMArray::CopyAndExtendWith(vm_oop_t item) const {
    size_t fields = GetNumberOfIndexableFields();
    VMArray* result = GetUniverse()->NewArray(fields + 1);
    CopyIndexableFieldsTo(result);
    result->SetIndexableField(fields, item);
    return result;
}

VMArray* VMArray::Clone() const {
    long addSpace = objectSize - sizeof(VMArray);
    VMArray* clone = new (GetHeap<HEAP_CLS>(), addSpace ALLOC_MATURE) VMArray(*this);
    void* destination  = SHIFTED_PTR(clone, sizeof(VMArray));
    const void* source = SHIFTED_PTR(this, sizeof(VMArray));
    size_t noBytes = GetObjectSize() - sizeof(VMArray);
    memcpy(destination, source, noBytes);
    return clone;
}

void VMArray::MarkObjectAsInvalid() {
    VMObject::MarkObjectAsInvalid();
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        FIELDS[i] = INVALID_GC_POINTER;
    }
}

void VMArray::CopyIndexableFieldsTo(VMArray* to) const {
    long numIndexableFields = GetNumberOfIndexableFields();
    for (long i = 0; i < numIndexableFields; ++i) {
        to->SetIndexableField(i, GetIndexableField(i));
    }
}

void VMArray::WalkObjects(walk_heap_fn walk) {
    clazz = static_cast<GCClass*>(walk(clazz));
    long numFields          = GetNumberOfFields();
    long numIndexableFields = GetNumberOfIndexableFields();
    gc_oop_t* fields = FIELDS;
    for (long i = 0; i < numFields + numIndexableFields; i++) {
        fields[i] = walk(fields[i]);
    }
}

StdString VMArray::AsDebugString() const {
    return "Array(" + to_string(GetNumberOfIndexableFields()) + ")";
}
