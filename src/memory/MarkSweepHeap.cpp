#include "MarkSweepHeap.h"

#include <string.h>

#include "MarkSweepCollector.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

MarkSweepHeap::MarkSweepHeap(long objectSpaceSize) : Heap<MarkSweepHeap>(new MarkSweepCollector(this), objectSpaceSize) {
    //our initial collection limit is 90% of objectSpaceSize
    collectionLimit = objectSpaceSize * 0.9;
    spcAlloc = 0;
    allocatedObjects = new vector<AbstractVMObject*>();
}

AbstractVMObject* MarkSweepHeap::AllocateObject(size_t size) {
    //TODO: PADDING wird eigentlich auch durch malloc erledigt
    AbstractVMObject* newObject = (AbstractVMObject*) malloc(size);
    if (newObject == nullptr) {
        Universe::ErrorPrint("Failed to allocate " + to_string(size) + " Bytes.\n");
        GetUniverse()->Quit(-1);
    }
    spcAlloc += size;
    memset((void*) newObject, 0, size);
    //AbstractObjects (Integer,...) have no Size field anymore -> set within VMObject's new operator
    //newObject->SetObjectSize(size);
    allocatedObjects->push_back(newObject);
    //let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit)
        triggerGC();
    return newObject;
}
