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

VMObject::VMObject(long numberOfFields) {
    // this line would be needed if the VMObject** is used instead of the macro:
    // FIELDS = (pVMObject*)&clazz;
    this->SetNumberOfFields(numberOfFields + VMObjectNumberOfFields);
    gcfield = 0;
    hash = (size_t) this;
    // Object size was already set by the heap on allocation
    
#if GC_TYP==PAUSELESS
    newClone = NULL;
    pthread_mutex_init(&beingMovedMutex, NULL);
    pthread_cond_init(&beingMovedCondition, NULL);
#endif
}

#if GC_TYPE==GENERATIONAL
pVMObject VMObject::Clone() {
    VMObject* clone = new (_HEAP, _PAGE, objectSize - sizeof(VMObject), true) VMObject(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
            SHIFTED_PTR(this,sizeof(VMObject)), GetObjectSize() -
            sizeof(VMObject));
    clone->hash = (size_t) &clone;
    return clone;
}
#elif GC_TYPE==PAUSELESS
pVMObject VMObject::Clone(Page* page) {
    VMObject* clone = new (page, objectSize - sizeof(VMObject)) VMObject(*this);
    memcpy(&(clone->clazz), &clazz, objectSize - sizeof(VMObject) + sizeof(pVMObject));
    clone->hash = (size_t) &clone;
    return clone;
}
#else
pVMObject VMObject::Clone() {
    VMObject* clone = new (_HEAP, objectSize - sizeof(VMObject)) VMObject(*this);
    memcpy(&(clone->clazz), &clazz,
           objectSize - sizeof(VMObject) + sizeof(pVMObject));
    clone->hash = (size_t) &clone;
    return clone;
}
#endif

void VMObject::SetNumberOfFields(long nof) {
    this->numberOfFields = nof;
    // initialize fields with NilObject
    for (long i = 0; i < nof; ++i) {
        FIELDS[i] = WRITEBARRIER(READBARRIER(nilObject));
    }
}

void VMObject::SetClass(pVMClass cl) {
    clazz = WRITEBARRIER(cl);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, cl);
#endif
}

pVMSymbol VMObject::GetFieldName(long index) /*const*/ {
    //return this->clazz->GetInstanceFieldName(index);
    //because we want to make sure that the ReadBarrier is triggered
    return this->GetClass()->GetInstanceFieldName(index);
}

void VMObject::Assert(bool value) const {
    _UNIVERSE->Assert(value);
}

pVMObject VMObject::GetField(long index) /*const*/ {
    return READBARRIER(FIELDS[index]);
}

void VMObject::SetField(long index, pVMObject value) {
    FIELDS[index] = WRITEBARRIER(value);
#if GC_TYPE==GENERATIONAL
    _HEAP->WriteBarrier(this, (VMOBJECT_PTR)value);
#endif
}

//returns the Object's additional memory used (e.g. for Array fields)
long VMObject::GetAdditionalSpaceConsumption() const {
    //The VM*-Object's additional memory used needs to be calculated.
    //It's      the total object size   MINUS   sizeof(VMObject) for basic
    //VMObject  MINUS   the number of fields times sizeof(pVMObject)
    return (objectSize
            - (sizeof(VMObject)
                    + sizeof(pVMObject) * GetNumberOfFields()));
}

void VMObject::MarkObjectAsInvalid() {
    clazz = (pVMClass) INVALID_POINTER;
}

#if GC_TYPE==PAUSELESS
void VMObject::MarkReferences() {
    ReadBarrierForGCThread((void**)&clazz);
    long numFields = GetNumberOfFields();
    for (long i = 0; i < numFields; ++i) {
        ReadBarrierForGCThread((void**)&FIELDS[i]);
    }
}
#else
void VMObject::WalkObjects(VMOBJECT_PTR (*walk)(VMOBJECT_PTR)) {
    //if (clazz == threadClass) {
    //    int i = 1;
    //}
    
    clazz = (pVMClass) walk(clazz);
    
    long numFields = GetNumberOfFields();
    for (long i = 0; i < numFields; ++i) {
        FIELDS[i] = walk((VMOBJECT_PTR)GetField(i));
    }
}
#endif
