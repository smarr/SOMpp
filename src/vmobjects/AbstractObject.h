#pragma once

/*
 * AbstractVMObject.h
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include <assert.h>
#include <iostream>


#include <misc/defs.h>
#include <memory/GenerationalHeap.h>
#include <memory/CopyingHeap.h>
#include <memory/MarkSweepHeap.h>

#include "ObjectFormats.h"
#include "VMObjectBase.h"

/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PADDED_SIZE(N) ((((uint32_t)N)+(sizeof(void*)-1) & ~(sizeof(void*)-1)))

using namespace std;

//this is the base class for all VMObjects
class AbstractVMObject: public VMObjectBase {
public:
    typedef GCAbstractObject Stored;
    
    virtual int64_t GetHash();
    virtual VMClass* GetClass() const = 0;
    virtual AbstractVMObject* Clone() const = 0;
    virtual void Send(Interpreter*, StdString, vm_oop_t*, long);
    virtual size_t GetObjectSize() const = 0;
    
    virtual void MarkObjectAsInvalid() = 0;
    
    virtual StdString AsDebugString() const = 0;

    AbstractVMObject() {
        gcfield = 0;
    }

    inline virtual void SetObjectSize(size_t size) {
        Universe::ErrorPrint("this object doesn't support SetObjectSize\n");
        throw "this object doesn't support SetObjectSize";
    }

    inline virtual long GetNumberOfFields() const {
        Universe::ErrorPrint("this object doesn't support GetNumberOfFields\n");
        throw "this object doesn't support GetNumberOfFields";
    }

    virtual void SetNumberOfFields(long nof) {
        Universe::ErrorPrint("this object doesn't support SetNumberOfFields\n");
        throw "this object doesn't support SetNumberOfFields";
    }
    inline virtual void SetClass(VMClass* cl) {
        Universe::ErrorPrint("this object doesn't support SetClass\n");
        throw "this object doesn't support SetClass";
    }

    long GetFieldIndex(VMSymbol* fieldName) const;

    inline virtual void WalkObjects(walk_heap_fn) {
        return;
    }

    inline virtual VMSymbol* GetFieldName(long index) const {
        Universe::ErrorPrint("this object doesn't support GetFieldName\n");
        throw "this object doesn't support GetFieldName";
    }

    void* operator new(size_t numBytes, HEAP_CLS* heap,
            unsigned long additionalBytes = 0 ALLOC_OUTSIDE_NURSERY_DECL) {
        // if outsideNursery flag is set or object is too big for nursery, we
        // allocate a mature object
        unsigned long add = PADDED_SIZE(additionalBytes);
        void* result;
#if GC_TYPE==GENERATIONAL
        if (outsideNursery) {
            result = (void*) heap->AllocateMatureObject(numBytes + add);
        } else {
            result = (void*) heap->AllocateNurseryObject(numBytes + add);
        }
#else
        result = (void*) heap->AllocateObject(numBytes + add);
#endif

        assert(result != INVALID_VM_POINTER);
        return result;
    }

};
