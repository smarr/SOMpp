#pragma once

/*
 * AbstractVMObject.h
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include <cassert>
#include <iostream>

#include "../memory/CopyingHeap.h"
#include "../memory/DebugCopyingHeap.h"
#include "../memory/GenerationalHeap.h"
#include "../memory/MarkSweepHeap.h"
#include "../misc/defs.h"
#include "../vm/Print.h"
#include "ObjectFormats.h"
#include "VMObjectBase.h"

/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PADDED_SIZE(N) ((((uint32_t)(N))+(sizeof(void*)-1) & ~(sizeof(void*)-1)))

using namespace std;

class Interpreter;

//this is the base class for all VMObjects
class AbstractVMObject: public VMObjectBase {
public:
    typedef GCAbstractObject Stored;

    virtual int64_t GetHash() const;
    virtual VMClass* GetClass() const = 0;
    virtual AbstractVMObject* Clone() const = 0;
            void Send(Interpreter*, StdString, vm_oop_t*, long);

    /** Size in bytes of the object. */
    virtual size_t GetObjectSize() const = 0;

    virtual void MarkObjectAsInvalid() = 0;
    virtual bool IsMarkedInvalid() const = 0;

    virtual StdString AsDebugString() const = 0;

    AbstractVMObject() {
        gcfield = 0;
    }
    ~AbstractVMObject() override = default;

    inline virtual void SetObjectSize(size_t) {
        ErrorPrint("this object doesn't support SetObjectSize\n");
    }

    inline virtual long GetNumberOfFields() const {
        ErrorPrint("this object doesn't support GetNumberOfFields\n");
        return -1;
    }

    inline virtual void SetClass(VMClass*) {
        ErrorPrint("this object doesn't support SetClass\n");
    }

    long GetFieldIndex(VMSymbol* fieldName) const;

    virtual void WalkObjects(walk_heap_fn) {}

    inline virtual VMSymbol* GetFieldName(long) const {
        ErrorPrint("this object doesn't support GetFieldName\n");
        return nullptr;
    }

    void* operator new(size_t numBytes, HEAP_CLS* heap,
            unsigned long additionalBytes = 0 ALLOC_OUTSIDE_NURSERY_DECL) {
        // if outsideNursery flag is set or object is too big for nursery, we
        // allocate a mature object
        const unsigned long add = PADDED_SIZE(additionalBytes);
        void* result = nullptr;
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
