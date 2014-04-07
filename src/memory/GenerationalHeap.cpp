#include "GenerationalHeap.h"
#include "GenerationalCollector.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

#include <string.h>
#include <iostream>

#include <sys/mman.h>

#if GC_TYPE == GENERATIONAL

using namespace std;


GenerationalHeap::GenerationalHeap(long objectSpaceSize) {
    //our initial collection limit is 90% of objectSpaceSize
    //collectionLimit = objectSpaceSize * 0.9;
    //gc = new GenerationalCollector(this);
    
    // create region in which we can allocate objects, pages will be created in this region
    nurseryStart = mmap(NULL, objectSpaceSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    memset(nurseryStart, 0x0, objectSpaceSize);
    // initialize some meta data of the heap
    nextFreePagePosition = nurseryStart;
    availablePages = new vector<Page*>();
    fullPages = new vector<Page*>();
}

Page* GenerationalHeap::RequestPage() {
    Page* newPage;
    if (availablePages->empty()) {
        newPage = new Page(nextFreePagePosition);
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

#endif




/*
 
 
 #define DISPATCH_GC() {\
 if (_HEAP->isCollectionTriggered()) {\
 _FRAME->SetBytecodeIndex(bytecodeIndexGlobal);\
 _HEAP->FullGC();\
 method = _FRAME->GetMethod(); \
 currentBytecodes = method->GetBytecodes(); \
 }\
 goto *loopTargets[currentBytecodes[bytecodeIndexGlobal]];\
 }
 
*/





/*
GenerationalHeap::GenerationalHeap(long objectSpaceSize) {
    //our initial collection limit is 90% of objectSpaceSize
    //collectionLimit = objectSpaceSize * 0.9;
    gc = new GenerationalCollector(this);

    nursery = malloc(objectSpaceSize);
    nurserySize = objectSpaceSize;
    maxNurseryObjSize = objectSpaceSize / 2;
    nursery_end = (size_t)nursery + nurserySize;
    matureObjectsSize = 0;
    memset(nursery, 0x0, objectSpaceSize);
    collectionLimit = (void*)((size_t)nursery + ((size_t)(objectSpaceSize *
                            0.9)));
    nextFreePosition = nursery;
    allocatedObjects = new vector<VMOBJECT_PTR>();
    oldObjsWithRefToYoungObjs = new vector<size_t>();
}

AbstractVMObject* GenerationalHeap::AllocateNurseryObject(size_t size) {
    pthread_mutex_lock(&allocationLock);
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if ((size_t)nextFreePosition > nursery_end) {
        cout << "Failed to allocate " << size << " Bytes in nursery." << endl;
        _UNIVERSE->Quit(-1);
    }
    //let's see if we have to trigger the GC
    if (nextFreePosition > collectionLimit)
        triggerGC();
    pthread_mutex_unlock(&allocationLock);
    return newObject;
}

AbstractVMObject* GenerationalHeap::AllocateMatureObject(size_t size) {
    pthread_mutex_lock(&allocationLock);
    VMOBJECT_PTR newObject = (VMOBJECT_PTR)malloc(size);
    if (newObject == NULL) {
        cout << "Failed to allocate " << size << " Bytes." << endl;
        _UNIVERSE->Quit(-1);
    }
    allocatedObjects->push_back(newObject);
    matureObjectsSize += size;
    pthread_mutex_unlock(&allocationLock);
    return newObject;
}

void GenerationalHeap::writeBarrier_OldHolder(VMOBJECT_PTR holder, const VMOBJECT_PTR
        referencedObject) {
    if (isObjectInNursery(referencedObject)) {
        oldObjsWithRefToYoungObjs->push_back((size_t)holder);
        holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
    }
}

#endif
*/