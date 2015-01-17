#pragma once

#include <assert.h>

/*
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

#include <vector>
#include <iostream>
#include <cstring>

#include "AbstractObject.h"

#include <misc/defs.h>
#include <vm/Universe.h>

#if GC_TYPE==PAUSELESS
#include <memory/pauseless/PauselessHeap.h>
#include <pthread.h>
#endif

//#include "ObjectFormats.h"

// this macro returns a shifted ptr by offset bytes
#define SHIFTED_PTR(ptr, offset) ((void*)((size_t)(ptr)+(size_t)(offset)))

/* chbol: this table is not correct anymore because of introduction of
 * class AbstractVMObject
 **************************VMOBJECT****************************
 * __________________________________________________________ *
 *| vtable*          |   0x00 - 0x03                         |*
 *|__________________|_______________________________________|*
 *| hash             |   0x04 - 0x07                         |*
 *| objectSize       |   0x08 - 0x0b                         |*
 *| numberOfFields   |   0x0c - 0x0f                         |*
 *| gcField          |   0x10 - 0x13 (because of alignment)  |*
 *| clazz            |   0x14 - 0x17                         |*
 *|__________________|___0x18________________________________|*
 *                                                            *
 **************************************************************
 */

// FIELDS starts indexing after the clazz field
#define FIELDS (((gc_oop_t*)&clazz) + 1)

class VMObject: public AbstractVMObject {

public:
    typedef GCObject Stored;
    
    VMObject(long numberOfFields = 0);

    virtual inline VMClass*  GetClass();
    virtual        void      SetClass(VMClass* cl);
    virtual        VMSymbol* GetFieldName(long index);
    virtual inline long      GetNumberOfFields() const;
    virtual        void      SetNumberOfFields(long nof);
            inline vm_oop_t  GetField(long index);
            inline void      SetField(long index, vm_oop_t value);
    virtual        void      Assert(bool value) const;
    virtual inline size_t    GetObjectSize() const;
    virtual inline void      SetObjectSize(size_t size);
    
    virtual        void      MarkObjectAsInvalid();

#if GC_TYPE==PAUSELESS
    virtual        VMObject* Clone(Interpreter*);
    virtual        VMObject* Clone(PauselessCollectorThread*);
    virtual        void      MarkReferences();
    virtual        void      CheckMarking(void (vm_oop_t));
#else
    virtual        VMObject* Clone();
    virtual        void      WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR));
#endif
    
    virtual        StdString AsDebugString();

    /**
     * usage: new( <heap> [, <additional_bytes>] ) VMObject( <constructor params> )
     * num_bytes parameter is set by the compiler.
     * parameter additional_bytes (a_b) is used for:
     *   - fields in VMObject, a_b must be set to (numberOfFields*sizeof(VMObject*))
     *   - chars in VMString/VMSymbol, a_b must be set to (Stringlength + 1)
     *   - array size in VMArray; a_b must be set to (size_of_array*sizeof(VMObect*))
     *   - fields in VMMethod, a_b must be set to (number_of_bc + number_of_csts*sizeof(VMObject*))
     */
    void* operator new(size_t numBytes,
#if GC_TYPE==GENERATIONAL
        PagedHeap* heap, Page* page, unsigned long additionalBytes = 0, bool outsideNursery = false) {
            void* mem = AbstractVMObject::operator new(numBytes, heap, page, PADDED_SIZE(additionalBytes), outsideNursery);
#elif GC_TYPE==COPYING
        PagedHeap* heap, unsigned long additionalBytes = 0) {
            void* mem = (void*) ((CopyingHeap*)heap)->AllocateObject(numBytes + PADDED_SIZE(additionalBytes));
#elif GC_TYPE==MARK_SWEEP
        PagedHeap* heap, unsigned long additionalBytes = 0) {
            void* mem = (void*) ((MarkSweepHeap*)heap)->AllocateObject(numBytes + PADDED_SIZE(additionalBytes));
#elif GC_TYPE==PAUSELESS
        PagedHeap* heap, Interpreter* thread, unsigned long additionalBytes = 0, bool notRelocated = false) {
            void* mem = AbstractVMObject::operator new(numBytes, heap, thread, PADDED_SIZE(additionalBytes), notRelocated);
#endif
        size_t objSize = numBytes + PADDED_SIZE(additionalBytes);
        ((VMObject*) mem)->objectSize = objSize;
        assert(mem != INVALID_VM_POINTER);
        return mem;
    }

          
#if GC_TYPE==PAUSELESS
    void* operator new(size_t numBytes, PagedHeap* heap, PauselessCollectorThread* thread, unsigned long additionalBytes = 0, bool notRelocated = false) {
        void* mem = AbstractVMObject::operator new(numBytes, heap, thread, PADDED_SIZE(additionalBytes), notRelocated);
        size_t objSize = numBytes + PADDED_SIZE(additionalBytes);
        ((VMObject*) mem)->objectSize = objSize;
        assert(mem != INVALID_VM_POINTER);
        return mem;
    }
#endif
            
            
protected:
    long GetAdditionalSpaceConsumption() const;

    // VMObject essentials
    intptr_t hash;
    size_t objectSize;     // set by the heap at allocation time
    long   numberOfFields;

    GCClass* clazz;

    // Start of fields. All members beyond after clazz are indexable.
    // clazz has index -1.
    
private:
    static const long VMObjectNumberOfFields;
};

size_t VMObject::GetObjectSize() const {
    return objectSize;
}

void VMObject::SetObjectSize(size_t size) {
    objectSize = size;
}

VMClass* VMObject::GetClass() {
    VMClass* res = load_ptr(clazz);
    assert(Universe::IsValidObject(reinterpret_cast<VMObject*>(res)));
    return res;
}

long VMObject::GetNumberOfFields() const {
    return numberOfFields;
}

vm_oop_t VMObject::GetField(long index) {
    return load_ptr(FIELDS[index]);
}

void VMObject::SetField(long index, vm_oop_t value) {
    assert(Universe::IsValidObject(value));
    store_ptr(FIELDS[index], value);
}
