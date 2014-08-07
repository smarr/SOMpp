#include "GenerationalHeap.h"
#include "GenerationalCollector.h"
//#include "Page.h"
#include "../../vmobjects/AbstractObject.h"
#include "../../vm/Universe.h"

#include <string.h>
#include <iostream>

#include <sys/mman.h>

#if GC_TYPE == GENERATIONAL

GenerationalHeap::GenerationalHeap(long objectSpaceSize, long pageSize) {
    //our initial collection limit is 90% of objectSpaceSize
    gc = new GenerationalCollector(this);
    // meta data for mature object allocation
    pthread_mutex_init(&allocationLock, NULL);
    matureObjectsSize = 0;
    allocatedObjects = new vector<VMOBJECT_PTR>();
    oldObjsWithRefToYoungObjs = new vector<size_t>();
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
        // TODO: thread local mark table??? cross generation pointer vector...
        
        oldObjsWithRefToYoungObjs->push_back((size_t)holder);
        holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
    }
}

#endif
