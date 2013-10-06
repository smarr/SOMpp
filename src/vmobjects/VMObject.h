#pragma once
#ifndef VMOBJECT_H_
#define VMOBJECT_H_

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

#include "../misc/defs.h"
#include "../memory/Heap.h"
#include "../vm/Universe.h"

#include "ObjectFormats.h"

class VMSymbol;
class VMClass;

#define FIELDS ((pVMObject*)&clazz)

/*
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

class VMObject {

public:
    /* Constructor */
    VMObject(long numberOfFields = 0);

    /* Virtual member functions */
    virtual pVMClass GetClass() const;
    virtual void SetClass(pVMClass cl);
    virtual pVMSymbol GetFieldName(long index) const;
    virtual long GetFieldIndex(pVMSymbol fieldName) const;
    virtual long GetNumberOfFields() const;
    virtual void SetNumberOfFields(long nof);
    virtual long GetDefaultNumberOfFields() const;
    virtual void Send(StdString, pVMObject*, long);
    virtual pVMObject GetField(long index) const;
    virtual void Assert(bool value) const;
    virtual void SetField(long index, pVMObject value);
    virtual void MarkReferences();

    intptr_t GetHash() const {return hash;};
    size_t GetObjectSize() const;
    int32_t GetGCField() const;
    void SetGCField(int32_t value);
    void SetObjectSize(size_t size);

    /* Operators */

    /**
     * usage: new( <heap> [, <additional_bytes>] ) VMObject( <constructor params> )
     * num_bytes parameter is set by the compiler.
     * parameter additional_bytes (a_b) is used for:
     *   - fields in VMObject, a_b must be set to (numberOfFields*sizeof(pVMObject))
     *   - chars in VMString/VMSymbol, a_b must be set to (Stringlength + 1)
     *   - array size in VMArray; a_b must be set to (size_of_array*sizeof(VMObect*))
     *   - fields in VMMethod, a_b must be set to (number_of_bc + number_of_csts*sizeof(pVMObject))
     */
    void* operator new(size_t numBytes, Heap* heap,
            unsigned long additionalBytes = 0) {
        void* mem = (void*)heap->AllocateObject(numBytes + additionalBytes);
        return mem;
    }

    void operator delete(void* self, Heap* heap,
            unsigned int /*additional_bytes*/) {
        heap->Destroy((VMObject*)self);
    }

    void operator delete( void* self, Heap* heap) {
        heap->Destroy((VMObject*)self);
    }

protected:
    long GetAdditionalSpaceConsumption() const;
    //VMObject essentials
    intptr_t hash;
    size_t objectSize; //set by the heap at allocation time
    long numberOfFields;
    int32_t gcfield;

    //pVMObject* FIELDS;
    //Start of fields. All members beyond this point are indexable
    //through FIELDS-macro instead of the member above.
    //So clazz == FIELDS[0]
    pVMClass clazz;
private:
    static const long VMObjectNumberOfFields;
};

#endif
