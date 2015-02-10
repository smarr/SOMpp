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
#include <memory/GenerationalPage.h>
#include <memory/CopyingHeap.h>
#include <memory/CopyingPage.h>
#include <memory/MarkSweepHeap.h>
#include <memory/MarkSweepPage.h>

#include "VMObjectBase.h"

#include <vm/Universe.h>

/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PADDED_SIZE(N) ((((size_t)(N))+(sizeof(void*)-1) & ~(sizeof(void*)-1)))

using namespace std;

// this is the base class for all VMObjects
class AbstractVMObject: public VMObjectBase {
public:
    typedef GCAbstractObject Stored;
    
    virtual intptr_t GetHash();
    virtual VMClass* GetClass() = 0;
    virtual AbstractVMObject* Clone(Page*) = 0;
    virtual void Send(Interpreter*, StdString, vm_oop_t*, long);
    virtual size_t GetObjectSize() const = 0;
    
    virtual void MarkObjectAsInvalid() = 0;
    
    virtual StdString AsDebugString() = 0;

    AbstractVMObject() : VMObjectBase() {}

    inline virtual void SetClass(VMClass* cl) {
        Universe::ErrorPrint("this object doesn't support SetClass\n");
        throw "this object doesn't support SetClass";
    }

    long GetFieldIndex(VMSymbol* fieldName);

    inline virtual VMSymbol* GetFieldName(long index) const {
        Universe::ErrorPrint("this object doesn't support GetFieldName\n");
        throw "this object doesn't support GetFieldName";
    }

    inline virtual void WalkObjects(walk_heap_fn, Page*) {
        return;
    }

    void* operator new(size_t numBytes, Page* page,
            unsigned long additionalBytes = 0 ALLOC_OUTSIDE_NURSERY_DECL) {
        size_t total = PADDED_SIZE(numBytes + additionalBytes);
        void* result = page->AllocateObject(total ALLOC_HINT);

        assert(result != INVALID_VM_POINTER);
        return result;
    }

};
