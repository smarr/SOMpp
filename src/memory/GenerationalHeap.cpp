#include "GenerationalHeap.h"
#include "GenerationalCollector.h"
#include "Page.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

#include <string.h>
#include <iostream>

#include <sys/mman.h>

#if GC_TYPE == GENERATIONAL

GenerationalHeap::GenerationalHeap(long objectSpaceSize, long pageSize) {
    //our initial collection limit is 90% of objectSpaceSize
    //collectionLimit = objectSpaceSize * 0.9;
    gc = new GenerationalCollector(this);
    // create region in which we can allocate objects, pages will be created in this region
    nurseryStart = mmap(NULL, objectSpaceSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    memset(nurseryStart, 0x0, objectSpaceSize);
    // initialize some meta data of the heap
    maxNurseryObjSize = pageSize / 2;
    nextFreePagePosition = nurseryStart;
    allPages = new vector<Page*>();
    availablePages = new vector<Page*>();
    fullPages = new vector<Page*>();
    collectionLimit = (void*)((size_t)nurseryStart + ((size_t)(objectSpaceSize * 0.9)));
    // meta data for mature object allocation
    pthread_mutex_init(&allocationLock, NULL);
    matureObjectsSize = 0;
    allocatedObjects = new vector<VMOBJECT_PTR>();
    oldObjsWithRefToYoungObjs = new vector<size_t>();
}

size_t GenerationalHeap::GetMaxNurseryObjectSize() {
    return maxNurseryObjSize;
}

Page* GenerationalHeap::RequestPage() {
    Page* newPage;
    if (availablePages->empty()) {
        newPage = new Page(nextFreePagePosition, this);
        allPages->push_back(newPage);
        nextFreePagePosition = (void*) ((size_t)nextFreePagePosition + pageSize);
        if (nextFreePagePosition > collectionLimit)
            triggerGC();
    } else {
        newPage = availablePages->back();
        availablePages->pop_back();
    }
    return newPage;
}

void GenerationalHeap::RelinquishPage(Page* page) {
    availablePages->push_back(page);
}

void GenerationalHeap::RelinquishFullPage(Page* page) {
    fullPages->push_back(page);
}

AbstractVMObject* GenerationalHeap::AllocateMatureObject(size_t size) {
    VMOBJECT_PTR newObject = (VMOBJECT_PTR)malloc(size);
    if (newObject == NULL) {
        cout << "Failed to allocate " << size << " Bytes." << endl;
        _UNIVERSE->Quit(-1);
    }
    pthread_mutex_lock(&allocationLock);
    allocatedObjects->push_back(newObject);
    matureObjectsSize += size;
    pthread_mutex_unlock(&allocationLock);
    return newObject;
}

void GenerationalHeap::WriteBarrierOldHolder(VMOBJECT_PTR holder, const VMOBJECT_PTR referencedObject) {
    if (IsObjectInNursery(referencedObject)) {
        oldObjsWithRefToYoungObjs->push_back((size_t)holder);
        holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
    }
}

#endif
