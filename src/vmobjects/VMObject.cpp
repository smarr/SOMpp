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
const size_t VMObject::VMObjectNumberOfGcPtrFields = 0;

VMObject::VMObject(long numberOfFields) : AbstractVMObject() {
    // this line would be needed if the VMObject** is used instead of the macro:
    // FIELDS = (VMObject**)&clazz;
    SetNumberOfFields(numberOfFields + VMObjectNumberOfFields);
    #if GC_TYPE==PAUSELESS
     gcfield2 = 0;
    #endif
    hash = (intptr_t) this;
    // Object size was already set by the heap on allocation
}

VMObject* VMObject::Clone(Page* page) {
    VMObject* clone = new (page, objectSize - sizeof(VMObject) ALLOC_MATURE) VMObject(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this,  sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->hash = (size_t) &clone;
    return clone;
}

void VMObject::SetNumberOfFields(long nof) {
    numberOfFields = nof;
    // initialize fields with NilObject
    for (long i = 0; i < nof; ++i) {
#warning do we need to cylce through the barriers here?
        store_ptr(FIELDS[i], load_ptr(nilObject));
    }
}

void VMObject::SetClass(VMClass* cl) {
    assert(Universe::IsValidObject((AbstractVMObject*) cl));
    store_ptr(clazz, cl);
}

VMSymbol* VMObject::GetFieldName(long index) {
    return GetClass()->GetInstanceFieldName(index);
}

void VMObject::Assert(bool value) const {
    GetUniverse()->Assert(value);
}

//returns the Object's additional memory used (e.g. for Array fields)
long VMObject::GetAdditionalSpaceConsumption() const {
    //The VM*-Object's additional memory used needs to be calculated.
    //It's      the total object size   MINUS   sizeof(VMObject) for basic
    //VMObject  MINUS   the number of fields times sizeof(VMObject*)
    return (objectSize
            - (sizeof(VMObject)
                    + sizeof(VMObject*) * GetNumberOfFields()));
}

void VMObject::MarkObjectAsInvalid() {
    clazz = (GCClass*) INVALID_GC_POINTER;
}

#if GC_TYPE==PAUSELESS
void VMObject::MarkReferences() {
    ReadBarrierForGCThread(&clazz);
    size_t numFields = GetNumberOfFields();
    for (size_t i = 0; i < numFields; ++i) {
        ReadBarrierForGCThread(&FIELDS[i]);
    }
}
void VMObject::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(clazz) == GetHeap<HEAP_CLS>()->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
    size_t numFields = GetNumberOfFields();
    for (size_t i = 0; i < numFields; ++i) {
        assert(GetNMTValue(FIELDS[i]) == GetHeap<HEAP_CLS>()->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(FIELDS[i]));
        walk(Untag(FIELDS[i]));
    }
}
#else
void VMObject::WalkObjects(walk_heap_fn walk, Page* page) {
    clazz = static_cast<GCClass*>(walk(clazz, page));

    size_t numFields = GetNumberOfFields();
    for (size_t i = 0; i < numFields; ++i) {
        FIELDS[i] = walk(FIELDS[i], page);
    }
}
#endif

StdString VMObject::AsDebugString() {
    if (this == load_ptr(nilObject)) {
        return "nilObject";
    } else if (this == load_ptr(trueObject)) {
        return "trueObject";
    } else if (this == load_ptr(falseObject)) {
        return "falseObject";
    }
    return "Object(" + GetClass()->GetName()->GetStdString() + ")";
}
