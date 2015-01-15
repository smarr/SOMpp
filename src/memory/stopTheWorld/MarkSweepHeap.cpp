#include "MarkSweepHeap.h"

#if GC_TYPE == MARK_SWEEP

#include <string.h>

#include "MarkSweepCollector.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

MarkSweepHeap::MarkSweepHeap(long objectSpaceSize) {
    //our initial collection limit is 90% of objectSpaceSize
    collectionLimit = objectSpaceSize * 0.9;
    spcAlloc = 0;
    gc = new MarkSweepCollector(this);
    allocatedObjects = new vector<VMOBJECT_PTR>();
}

AbstractVMObject* MarkSweepHeap::AllocateObject(size_t size) {
    pthread_mutex_lock(&allocationLock);
    //TODO: PADDING wird eigentlich auch durch malloc erledigt
    AbstractVMObject* newObject = (AbstractVMObject*) malloc(size);
    if (newObject == NULL) {
        cout << "Failed to allocate " << size << " Bytes." << endl;
        GetUniverse()->Quit(-1);
    }
    spcAlloc += size;
    memset(newObject, 0, size);
    //AbstractObjects (Integer,...) have no Size field anymore -> set within VMObject's new operator
    //newObject->SetObjectSize(size);
    allocatedObjects->push_back(newObject);
    //let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit)
    triggerGC();
    pthread_mutex_unlock(&allocationLock);
    return newObject;
}

#endif
