#include "Heap.h"
#include "GenerationalHeap.h"
#include "GenerationalCollector.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

#include <string.h>
#include <iostream>

using namespace std;

GenerationalHeap::GenerationalHeap(long objectSpaceSize) : Heap<GenerationalHeap>(new GenerationalCollector(this), objectSpaceSize) {
    //our initial collection limit is 90% of objectSpaceSize
    //collectionLimit = objectSpaceSize * 0.9;

    nursery = malloc(objectSpaceSize);
    nurserySize = objectSpaceSize;
    maxNurseryObjSize = objectSpaceSize / 2;
    nursery_end = (size_t)nursery + nurserySize;
    matureObjectsSize = 0;
    memset(nursery, 0x0, objectSpaceSize);
    collectionLimit = (void*)((size_t)nursery + ((size_t)(objectSpaceSize *
                            0.9)));
    nextFreePosition = nursery;
    allocatedObjects = new vector<AbstractVMObject*>();
    oldObjsWithRefToYoungObjs = new vector<size_t>();
}

AbstractVMObject* GenerationalHeap::AllocateNurseryObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if ((size_t)nextFreePosition > nursery_end) {
        Universe::ErrorPrint("Failed to allocate " + to_string(size) + " Bytes in nursery.\n");
        GetUniverse()->Quit(-1);
    }
    //let's see if we have to trigger the GC
    if (nextFreePosition > collectionLimit)
        triggerGC();
    return newObject;
}

AbstractVMObject* GenerationalHeap::AllocateMatureObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) malloc(size);
    if (newObject == nullptr) {
        Universe::ErrorPrint("Failed to allocate " + to_string(size) + " Bytes.\n");
        GetUniverse()->Quit(-1);
    }
    allocatedObjects->push_back(newObject);
    matureObjectsSize += size;
    return newObject;
}

void GenerationalHeap::writeBarrier_OldHolder(AbstractVMObject* holder,
                                              const vm_oop_t referencedObject) {
    if (isObjectInNursery(referencedObject)) {
        oldObjsWithRefToYoungObjs->push_back((size_t)holder);
        holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
    }
}

