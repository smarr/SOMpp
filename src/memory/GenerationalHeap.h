#pragma once

#include "../misc/defs.h"
#include <assert.h>
#if GC_TYPE ==GENERATIONAL

#include "Page.h"
#include "PagedHeap.h"
#include "../vmobjects/VMObjectBase.h"

class GenerationalHeap : public PagedHeap {
    friend class GenerationalCollector;
    friend class Page;
    
public:
    GenerationalHeap(long objectSpaceSize = 4194304, long pageSize = 32768);

    Page* RequestPage();
    void RelinquishPage(Page*);
    void RelinquishFullPage(Page*);
    
    size_t GetMaxNurseryObjectSize();
    AbstractVMObject* AllocateMatureObject(size_t);
    
    void WriteBarrier(VMOBJECT_PTR holder, const VMOBJECT_PTR referencedObject);
    inline bool IsObjectInNursery(const pVMObject obj);

private:
    size_t maxNurseryObjSize;
    void* nurseryStart;
    void* nextFreePagePosition;
    void* collectionLimit;
    // data to keep track of nursery pages
    vector<Page*>* allPages;
    vector<Page*>* availablePages;
    vector<Page*>* fullPages;
    // data for mature allocation
    pthread_mutex_t allocationLock;
    size_t matureObjectsSize;
    vector<size_t>* oldObjsWithRefToYoungObjs;
    vector<VMOBJECT_PTR>* allocatedObjects;
    
    void WriteBarrierOldHolder(VMOBJECT_PTR holder, const VMOBJECT_PTR referencedObject);
};

inline bool GenerationalHeap::IsObjectInNursery(const pVMObject obj) {
    return (size_t) obj >= (size_t)nurseryStart && (size_t) obj < (size_t)nurseryStart;// + pageSize;
}

inline void GenerationalHeap::WriteBarrier(VMOBJECT_PTR holder, const VMOBJECT_PTR referencedObject) {
#ifdef DEBUG
    //XXX Disabled because of speed reasons --> causes some tests to fail
    //writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif
    
    size_t gcfield = *(((size_t*)holder)+1);
    if ((gcfield & 6 /* MASK_OBJECT_IS_OLD + MASK_SEEN_BY_WRITE_BARRIER */) == 2 /* MASK_OBJECT_IS_OLD */)
        WriteBarrierOldHolder(holder, referencedObject);
}

#endif






