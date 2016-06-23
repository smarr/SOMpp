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

#include "ObjectFormats.h"

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

    virtual int64_t GetHash() { return hash; }
    virtual inline VMClass*  GetClass() const;
    virtual        void      SetClass(VMClass* cl);
    virtual        VMSymbol* GetFieldName(long index) const;
    virtual inline long      GetNumberOfFields() const;
    virtual        void      SetNumberOfFields(long nof);
            inline vm_oop_t  GetField(long index) const;
            inline void      SetField(long index, vm_oop_t value);
    virtual        void      Assert(bool value) const;
    virtual        void      WalkObjects(walk_heap_fn walk);
    virtual        VMObject* Clone() const;
    virtual inline size_t    GetObjectSize() const;
    virtual inline void      SetObjectSize(size_t size);
    
    virtual        void      MarkObjectAsInvalid();
    
    virtual        StdString AsDebugString() const;

    /**
     * usage: new( <heap> [, <additional_bytes>] ) VMObject( <constructor params> )
     * num_bytes parameter is set by the compiler.
     * parameter additional_bytes (a_b) is used for:
     *   - fields in VMObject, a_b must be set to (numberOfFields*sizeof(VMObject*))
     *   - chars in VMString/VMSymbol, a_b must be set to (Stringlength + 1)
     *   - array size in VMArray; a_b must be set to (size_of_array*sizeof(VMObect*))
     *   - fields in VMMethod, a_b must be set to (number_of_bc + number_of_csts*sizeof(VMObject*))
     */
    void* operator new(size_t numBytes, HEAP_CLS* heap, unsigned long additionalBytes = 0 ALLOC_OUTSIDE_NURSERY_DECL) {
        void* mem = AbstractVMObject::operator new(numBytes, heap, additionalBytes ALLOC_OUTSIDE_NURSERY(outsideNursery));
        assert(mem != INVALID_VM_POINTER);
        
        ((VMObject*) mem)->objectSize = numBytes + PADDED_SIZE(additionalBytes);
        return mem;
    }

protected:
    inline long GetAdditionalSpaceConsumption() const;

    // VMObject essentials
    int64_t hash;
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

VMClass* VMObject::GetClass() const {
    assert(Universe::IsValidObject((VMObject*) load_ptr(clazz)));
    return load_ptr(clazz);
}

long VMObject::GetNumberOfFields() const {
    return numberOfFields;
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

vm_oop_t VMObject::GetField(long index) const {
    vm_oop_t result = load_ptr(FIELDS[index]);
    assert(Universe::IsValidObject(result));
    return result;
}

void VMObject::SetField(long index, vm_oop_t value) {
    assert(Universe::IsValidObject(value));
    store_ptr(FIELDS[index], value);
}
