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
    virtual AbstractVMObject* Clone(Interpreter*) = 0;
    virtual AbstractVMObject* Clone(PauselessCollectorThread*) = 0;
    
    inline virtual void MarkReferences() {
        return;
    }
    virtual void CheckMarking(void (vm_oop_t)) {
        return;
    }
#else
    virtual AbstractVMObject* Clone() = 0;
    
    inline virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR)) {
        return;
    }
#endif

#if GC_TYPE==GENERATIONAL
    void* operator new(size_t numBytes, PagedHeap* heap, Page* page, unsigned long additionalBytes = 0, bool outsideNursery = false) {
        //if outsideNursery flag is set or object is too big for nursery, we
        // allocate a mature object
        void* result;
        if (outsideNursery) {
            result = (void*) ((GenerationalHeap*)heap)->AllocateMatureObject(numBytes + additionalBytes);
        } else {
            result = (void*) (page->AllocateObject(numBytes + additionalBytes));
        }
        assert(result != INVALID_VM_POINTER);
        return result;
    }
#elif GC_TYPE==PAUSELESS
    //this should probably be cleaned up a bit
    void* operator new(size_t numBytes, PagedHeap* heap, Interpreter* thread, unsigned long additionalBytes = 0, bool notRelocated = false) {
        void* result;
        if (!notRelocated) {
            Page* page = thread->GetPage();
            result = (void*) (page->AllocateObject(numBytes + additionalBytes));
            if (page->Full()) {
                thread->AddFullPage(page);
                thread->SetPage(heap->RequestPage());
            }
        } else {
            Page* page = thread->GetNonRelocatablePage();
            result = (void*) (page->AllocateObject(numBytes + additionalBytes));
            if (page->Full()) {
                thread->AddFullNonRelocatablePage(page);
                thread->SetNonRelocatablePage(heap->RequestPage());
            }
        }
        assert(result != INVALID_VM_POINTER);
        return result;
    }
    
    void* operator new(size_t numBytes, PagedHeap* heap, PauselessCollectorThread* thread, unsigned long additionalBytes = 0, bool notRelocated = false) {
        Page* page = thread->GetPage();
        void* result = (void*) (page->AllocateObject(numBytes + additionalBytes));
        if (page->Full()) {
            heap->RelinquishPage(page);
            thread->SetPage(heap->RequestPage());
        }
        assert(result != INVALID_VM_POINTER);
        return result;
    }
#else
    void* operator new(size_t numBytes, HEAP_CLS* heap, unsigned long additionalBytes = 0) {
        void* mem = (void*) heap->AllocateObject(numBytes + additionalBytes);
        assert(mem != INVALID_VM_POINTER);
        return mem;
    }
#endif

};
