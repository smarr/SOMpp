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
    AbstractVMObject* newObject = reinterpret_cast<AbstractVMObject*>(malloc(size));
    if (newObject == nullptr) {
        Universe::ErrorExit(("Failed to allocate " + to_string(size) + " Bytes.\n").c_str());
    }
    spcAlloc += size;
    memset((void*) newObject, 0, size);
    
    {
        lock_guard<mutex> lock(allocatedObjects_mutex);
        allocatedObjects->push_back(newObject);
    }
    
    // let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit)
        triggerGC();
    return newObject;
}
