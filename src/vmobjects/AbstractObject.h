#pragma once

/*
 * AbstractVMObject.h
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include <misc/defs.h>

#include "ObjectFormats.h"

#if GC_TYPE==GENERATIONAL
    #include <memory/GenerationalHeap.h>
    #include <memory/Page.h>
#elif GC_TYPE==COPYING
    #include <memory/CopyingHeap.h>
#elif GC_TYPE==MARK_SWEEP
    #include <memory/MarkSweepHeap.h>
#elif GC_TYPE==PAUSELESS
    #include <memory/Page.h>
    #include <memory/PauselessHeap.h>
    #include <memory/PauselessCollectorThread.h>
    class Worklist;
#endif

#include "VMObjectBase.h"

#include <interpreter/Interpreter.h>
#include <vm/Universe.h>


/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PADDED_SIZE(N) ((((size_t)(N))+(sizeof(void*)-1) & ~(sizeof(void*)-1)))

class VMClass;
class VMObject;
class VMSymbol;

#include <iostream>
#include <assert.h>
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

    AbstractVMObject() : VMObjectBase() {
#if GC_TYPE==PAUSELESS
        gcfield2 = 0;
#endif
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

    long GetFieldIndex(VMSymbol* fieldName);
    
    inline virtual VMSymbol* GetFieldName(long index) const {
        Universe::ErrorPrint("this object doesn't support GetFieldName\n");
        throw "this object doesn't support GetFieldName";
    }
    
#if GC_TYPE==PAUSELESS
    inline virtual void MarkReferences() {
        return;
    }
    virtual void CheckMarking(void (vm_oop_t)) {
        return;
    }
#else
    inline virtual void WalkObjects(walk_heap_fn) {
        return;
    }
#endif

    void* operator new(size_t numBytes, Page* page,
            unsigned long additionalBytes = 0 ALLOC_OUTSIDE_NURSERY_DECL ALLOC_NON_RELOCATABLE_DECL) {
        size_t total = PADDED_SIZE(numBytes + additionalBytes);
        void* result = page->AllocateObject(total ALLOC_HINT RELOC_HINT);

        assert(result != INVALID_VM_POINTER);
        return result;
    }

};
