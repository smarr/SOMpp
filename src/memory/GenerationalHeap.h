#pragma once

#include "../misc/defs.h"
#include <assert.h>
#if GC_TYPE ==GENERATIONAL

#include "PagedHeap.h"
#include "../vmobjects/VMObjectBase.h"
#include "Page.h"

#include <vm/Universe.h>

class GenerationalHeap : public PagedHeap {
    friend class GenerationalCollector;

public:
    GenerationalHeap(long objectSpaceSize = 1048576);

    Page* RequestPage();
    void RelinquishPage(Page*);
    void RelinquishFullPage(Page*);

private:
    void* nurseryStart;
    void* nextFreePagePosition;
    vector<Page*>* availablePages;
    vector<Page*>* fullPages;
    
    
    
    
    
    size_t nursery_end;
    size_t nurserySize;
    size_t maxNurseryObjSize;
    size_t matureObjectsSize;
    void* volatile nextFreePosition;
    void writeBarrier_OldHolder(VMOBJECT_PTR holder, const VMOBJECT_PTR
            referencedObject);
    void* collectionLimit;
    vector<size_t>* oldObjsWithRefToYoungObjs;
    vector<VMOBJECT_PTR>* allocatedObjects;
};






#endif


















/*
inline bool GenerationalHeap::isObjectInNursery(const pVMObject obj) {
    assert(Universe::IsValidObject(obj));
    
    return (size_t) obj >= (size_t)nursery && (size_t) obj < nursery_end;
}

inline size_t GenerationalHeap::GetMaxNurseryObjectSize() {
    return maxNurseryObjSize;
}
 
*/

inline void GenerationalHeap::writeBarrier(VMOBJECT_PTR holder, const VMOBJECT_PTR referencedObject) {
#ifdef DEBUG
    //XXX Disabled because of speed reasons --> causes some tests to fail
    //writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif
    
    assert(Universe::IsValidObject(referencedObject));
    assert(Universe::IsValidObject(holder));

    size_t gcfield = *(((size_t*)holder)+1);
    if ((gcfield & 6 /* MASK_OBJECT_IS_OLD + MASK_SEEN_BY_WRITE_BARRIER */) == 2 /* MASK_OBJECT_IS_OLD */)
        writeBarrier_OldHolder(holder, referencedObject);
}


