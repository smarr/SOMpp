#include "GenerationalHeap.h"
#include "GenerationalCollector.h"
//#include "Page.h"
#include <vmobjects/AbstractObject.h>
#include <vm/Universe.h>

#include <string.h>
#include <iostream>

#include <sys/mman.h>

#if GC_TYPE == GENERATIONAL

GenerationalHeap::GenerationalHeap(long objectSpaceSize, long pageSize) : StopTheWorldHeap(objectSpaceSize, pageSize) {
    //our initial collection limit is 90% of objectSpaceSize
    gc = new GenerationalCollector(this);
    // meta data for mature object allocation
    pthread_mutex_init(&allocationLock, nullptr);
    pthread_mutex_init(&writeBarrierLock, nullptr);
    matureObjectsSize = 0;
    allocatedObjects = new vector<AbstractVMObject*>();
    oldObjsWithRefToYoungObjs = new vector<AbstractVMObject*>();
}

AbstractVMObject* GenerationalHeap::AllocateMatureObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) malloc(size);
    if (newObject == nullptr) {
        cout << "Failed to allocate " << size << " Bytes." << endl;
        GetUniverse()->Quit(-1);
    }
    pthread_mutex_lock(&allocationLock);
    allocatedObjects->push_back(newObject);
    matureObjectsSize += size;
    pthread_mutex_unlock(&allocationLock);
    return newObject;
}

void GenerationalHeap::WriteBarrierOldHolder(AbstractVMObject* holder, vm_oop_t referencedObject) {
    if (IsObjectInNursery(referencedObject)) {
        // TODO: thread local mark table??? cross generation pointer vector...
        
        pthread_mutex_lock(&writeBarrierLock);
        oldObjsWithRefToYoungObjs->push_back(holder);
        pthread_mutex_unlock(&writeBarrierLock);
        holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
    }
}

#endif
