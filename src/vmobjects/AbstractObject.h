/*
 * AbstractVMObject.h
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#ifndef ABSTRACTOBJECT_H_
#define ABSTRACTOBJECT_H_

#include <misc/defs.h>

#include "ObjectFormats.h"

#if GC_TYPE==GENERATIONAL
    #include <memory/stopTheWorld/GenerationalHeap.h>
    #include <memory/Page.h>
#elif GC_TYPE==COPYING
    #include <memory/stopTheWorld/CopyingHeap.h>
#elif GC_TYPE==MARK_SWEEP
    #include <memory/stopTheWorld/MarkSweepHeap.h>
#elif GC_TYPE==PAUSELESS
    #include <memory/pauseless/PauselessHeap.h>
    #include <memory/Page.h>
    class Worklist;
#endif

#include "VMObjectBase.h"


#if GC_TYPE==PAUSELSESS
#define MASK_OBJECT_NMT (1 << 1)
#define UNTAG_REFERENCE(REFERENCE) (((reinterpret_cast<uintptr_t>(REFERENCE) & MASK_OBJECT_NMT) == 1) ? (reinterpret_cast<uintptr_t>(REFERENCE) ^ MASK_OBJECT_NMT) : reinterpret_cast<uintptr_t>(REFERENCE))
#else
#define UNTAG_REFERENCE(REFERENCE) (REFERENCE)
#endif


/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PADDED_SIZE(N) ((((uint32_t)N)+(sizeof(void*)-1) & ~(sizeof(void*)-1)))

class VMClass;
class VMObject;
class VMSymbol;

#include <iostream>
#include <assert.h>
using namespace std;

//this is the base class for all VMObjects
class AbstractVMObject: public VMObjectBase {
public:
    virtual size_t GetHash();
    virtual pVMClass GetClass() /*const*/ = 0;
    virtual void Send(StdString, pVMObject*, long);
    virtual size_t GetObjectSize() const = 0;
    
    virtual void MarkObjectAsInvalid() = 0;

    AbstractVMObject() {
        gcfield = 0;
    }

    inline virtual void SetObjectSize(size_t size) {
        cout << "this object doesn't support SetObjectSize" << endl;
        throw "this object doesn't support SetObjectSize";
    }

    inline virtual long GetNumberOfFields() const {
        cout << "this object doesn't support GetNumberOfFields" << endl;
        throw "this object doesn't support GetNumberOfFields";
    }

    virtual void SetNumberOfFields(long nof) {
        cout << "this object doesn't support SetNumberOfFields" << endl;
        throw "this object doesn't support SetNumberOfFields";
    }
    inline virtual void SetClass(pVMClass cl) {
        cout << "this object doesn't support SetClass" << endl;
        throw "this object doesn't support SetClass";
    }

    long GetFieldIndex(pVMSymbol fieldName) /*const*/;
    
    inline virtual pVMSymbol GetFieldName(long index) const {
        cout << "this object doesn't support GetFieldName" << endl;
        throw "this object doesn't support GetFieldName";
    }
    
#if GC_TYPE==PAUSELESS
    virtual AbstractVMObject* ProtectedClone(Page*);
    virtual AbstractVMObject* Clone(Page*) = 0;
    
    inline virtual void MarkReferences(Worklist*) {
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
        assert(result != INVALID_POINTER);
        return result;
    }
#elif GC_TYPE==PAUSELESS
    void* operator new(size_t numBytes, Page* page, unsigned long additionalBytes = 0) {
        void* result = (void*) (page->AllocateObject(numBytes + additionalBytes));
        assert(result != INVALID_POINTER);
        return result;
    }
#else
    void* operator new(size_t numBytes, HEAP_CLS* heap, unsigned long additionalBytes = 0) {
        void* mem = (void*) heap->AllocateObject(numBytes + additionalBytes);
        assert(mem != INVALID_POINTER);
        return mem;
    }
#endif
    
private:
#if GC_TYPE==PAUSELESS
    pVMObject newClone;
    pthread_mutex_t beingMovedMutex;
    pthread_cond_t  beingMovedCondition;
#endif

};
#endif /* ABSTRACTOBJECT_H_ */
