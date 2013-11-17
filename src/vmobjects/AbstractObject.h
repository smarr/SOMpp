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
  #include <memory/GenerationalHeap.h>
#elif GC_TYPE==COPYING
  #include <memory/CopyingHeap.h>
#elif GC_TYPE==MARK_SWEEP
  #include <memory/MarkSweepHeap.h>
#endif

#include "VMObjectBase.h"

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
    virtual pVMClass GetClass() const = 0;
    virtual AbstractVMObject* Clone() const = 0;
    virtual void Send(StdString, pVMObject*, long);
    virtual size_t GetObjectSize() const = 0;

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

    long GetFieldIndex(pVMSymbol fieldName) const;

    inline virtual void SetField(long index, pVMObject value) {
        cout << "this object doesn't support SetField" << endl;
        throw "this object doesn't support SetField";
    }

    virtual pVMObject GetField(long index) const;

    inline virtual void WalkObjects(VMOBJECT_PTR (VMOBJECT_PTR)) {
        return;
    }

    inline virtual pVMSymbol GetFieldName(long index) const {
        cout << "this object doesn't support GetFieldName" << endl;
        throw "this object doesn't support GetFieldName";
    }

#if GC_TYPE==GENERATIONAL
    void* operator new(size_t numBytes, Heap* heap,
            unsigned long additionalBytes = 0, bool outsideNursery = false) {
        //if outsideNursery flag is set or object is too big for nursery, we
        // allocate a mature object
        void* result;
        if (outsideNursery) {
            result = (void*) ((GenerationalHeap*)heap)->AllocateMatureObject(numBytes + additionalBytes);
        } else {
            result = (void*) ((GenerationalHeap*)heap)->AllocateNurseryObject(numBytes + additionalBytes);
        }
        
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

};
#endif /* ABSTRACTOBJECT_H_ */
