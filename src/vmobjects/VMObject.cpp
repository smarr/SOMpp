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
#include "../interpreter/Interpreter.h"

// clazz is the only field of VMObject so
const long VMObject::VMObjectNumberOfFields = 0;

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

#if GC_TYPE==GENERATIONAL
VMObject* VMObject::Clone() {
    VMObject* clone = new (_HEAP, _PAGE, objectSize - sizeof(VMObject), true) VMObject(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this,  sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    clone->hash = (size_t) &clone;
    return clone;
}
#elif GC_TYPE==PAUSELESS
VMObject* VMObject::Clone(Interpreter* thread) {
    VMObject* clone = new (_HEAP, thread, objectSize - sizeof(VMObject)) VMObject(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    //memcpy(&(clone->clazz), &clazz, objectSize - sizeof(VMObject) + sizeof(VMObject*));
    
    clone->hash = (size_t) &clone;
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
VMObject* VMObject::Clone(PauselessCollectorThread* thread) {
    VMObject* clone = new (_HEAP, thread, objectSize - sizeof(VMObject)) VMObject(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)), SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() - sizeof(VMObject));
    //memcpy(&(clone->clazz), &clazz, objectSize - sizeof(VMObject) + sizeof(VMObject*));
    
    clone->hash = (size_t) &clone;
    /* clone->IncreaseVersion();
    this->MarkObjectAsInvalid(); */
    return clone;
}
#else
VMObject* VMObject::Clone() {
    VMObject* clone = new (_HEAP, objectSize - sizeof(VMObject)) VMObject(*this);
    
    memcpy(&(clone->clazz), &clazz, objectSize - sizeof(VMObject) + sizeof(VMObject*));
    
    clone->hash = (size_t) &clone;
    return clone;
}
#endif

void VMObject::SetNumberOfFields(long nof) {
    numberOfFields = nof;
    // initialize fields with NilObject
    for (long i = 0; i < nof; ++i) {
        FIELDS[i] = store_ptr(load_ptr(nilObject));
    }
}

void VMObject::SetClass(VMClass* cl) {
    assert(Universe::IsValidObject((AbstractVMObject*) cl));
    clazz = store_ptr(cl);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, cl);
#endif
}

VMSymbol* VMObject::GetFieldName(long index) /*const*/ {
    //return this->clazz->GetInstanceFieldName(index);
    //because we want to make sure that the ReadBarrier is triggered
    return this->GetClass()->GetInstanceFieldName(index);
}

void VMObject::Assert(bool value) const {
    GetUniverse()->Assert(value);
}

vm_oop_t VMObject::GetField(long index) /*const*/ {
    return load_ptr(FIELDS[index]);
}

void VMObject::SetField(long index, vm_oop_t value) {
    FIELDS[index] = store_ptr(value);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, (VMOBJECT_PTR)value);
#endif
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
    long numFields = GetNumberOfFields();
    for (long i = 0; i < numFields; ++i) {
        ReadBarrierForGCThread(&FIELDS[i]);
    }
}
void VMObject::CheckMarking(void (*walk)(vm_oop_t)) {
    assert(GetNMTValue(clazz) == _HEAP->GetGCThread()->GetExpectedNMT());
    CheckBlocked(Untag(clazz));
    walk(Untag(clazz));
    long numFields = GetNumberOfFields();
    for (long i = 0; i < numFields; ++i) {
        assert(GetNMTValue(AS_GC_POINTER(FIELDS[i])) == _HEAP->GetGCThread()->GetExpectedNMT());
        CheckBlocked(Untag(AS_GC_POINTER(FIELDS[i])));
        walk(Untag(AS_GC_POINTER(FIELDS[i])));
    }
}
#else
void VMObject::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    //if (clazz == threadClass) {
    //    int i = 1;
    //}
    
    clazz = (GCClass*) walk(load_ptr(clazz));
    
    long numFields = GetNumberOfFields();
    for (long i = 0; i < numFields; ++i) {
        FIELDS[i] = (GCAbstractObject*) walk((VMOBJECT_PTR)GetField(i));
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
