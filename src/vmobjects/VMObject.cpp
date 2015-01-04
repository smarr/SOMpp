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

// clazz is the only field of VMObject so
const long VMObject::VMObjectNumberOfFields = 0;

VMObject::VMObject(long numberOfFields) {
    // this line would be needed if the VMObject** is used instead of the macro:
    // FIELDS = (VMObject**)&clazz;
    SetNumberOfFields(numberOfFields + VMObjectNumberOfFields);
    hash = (size_t) this;
    // Object size was already set by the heap on allocation
}

VMObject* VMObject::Clone() const {
    VMObject* clone = new (GetHeap<HEAP_CLS>(), objectSize - sizeof(VMObject) ALLOC_MATURE) VMObject(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this,  sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->hash = (size_t) &clone;
    return clone;
}

void VMObject::SetNumberOfFields(long nof) {
    numberOfFields = nof;
    // initialize fields with NilObject
    for (long i = 0; i < nof; ++i)
        FIELDS[i] = nilObject;
}

void VMObject::SetClass(VMClass* cl) {
    store_ptr(clazz, cl);
}

VMSymbol* VMObject::GetFieldName(long index) const {
    return load_ptr(clazz)->GetInstanceFieldName(index);
}

void VMObject::Assert(bool value) const {
    GetUniverse()->Assert(value);
}

void VMObject::WalkObjects(walk_heap_fn walk) {
    clazz = static_cast<GCClass*>(walk(clazz));
    
    long numFields = GetNumberOfFields();
    for (long i = 0; i < numFields; ++i) {
# warning not sure whether the use of _store_ptr is correct and robust here
        FIELDS[i] = walk(_store_ptr(GetField(i)));
    }
}

void VMObject::MarkObjectAsInvalid() {
    clazz = (GCClass*) INVALID_GC_POINTER;
}

StdString VMObject::AsDebugString() const {
    if (this == load_ptr(nilObject)) {
        return "nilObject";
    } else if (this == load_ptr(trueObject)) {
        return "trueObject";
    } else if (this == load_ptr(falseObject)) {
        return "falseObject";
    }
    return "Object(" + GetClass()->GetName()->GetStdString() + ")";
}
