#pragma once

#include <misc/defs.h>
#include <assert.h>
#if GC_TYPE ==GENERATIONAL

#include "StopTheWorldHeap.h"
#include <vmobjects/VMObjectBase.h>

class GenerationalHeap : public StopTheWorldHeap {
    friend class GenerationalCollector;
    
public:
    GenerationalHeap(long, long);

    AbstractVMObject* AllocateMatureObject(size_t);
    
    void WriteBarrier(AbstractVMObject* holder, vm_oop_t referencedObject);
    inline bool IsObjectInNursery(vm_oop_t obj);

private:
    // data for mature allocation
    pthread_mutex_t allocationLock;
    pthread_mutex_t writeBarrierLock;
    size_t matureObjectsSize;
    vector<AbstractVMObject*>* oldObjsWithRefToYoungObjs;
    vector<AbstractVMObject*>* allocatedObjects;
    
    void WriteBarrierOldHolder(AbstractVMObject* holder, vm_oop_t referencedObject);
};

inline bool GenerationalHeap::IsObjectInNursery(vm_oop_t obj) {
    return (size_t) obj >= (size_t)memoryStart && (size_t) obj < memoryEnd;
}

inline void GenerationalHeap::WriteBarrier(AbstractVMObject* holder, vm_oop_t referencedObject) {
#ifdef DEBUG
    //XXX Disabled because of speed reasons --> causes some tests to fail
    //writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif
    
    size_t gcfield = *(((size_t*)holder)+1);
    if ((gcfield & 6 /* MASK_OBJECT_IS_OLD + MASK_SEEN_BY_WRITE_BARRIER */) == 2 /* MASK_OBJECT_IS_OLD */)
        WriteBarrierOldHolder(holder, referencedObject);
}

#endif






