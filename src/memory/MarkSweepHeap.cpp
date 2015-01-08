#include "MarkSweepHeap.h"

#include <string.h>

#include "MarkSweepCollector.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

__thread vector<AbstractVMObject*>* MarkSweepHeap::allocatedObjects;

MarkSweepHeap::MarkSweepHeap(long objectSpaceSize) : Heap<MarkSweepHeap>(new MarkSweepCollector(this), objectSpaceSize) {
    //our initial collection limit is 90% of objectSpaceSize
    collectionLimit = objectSpaceSize * 0.9;
    spcAlloc = 0;
}

# warning we never clean up the vector of thread-local vectors

void MarkSweepHeap::RegisterThread() {
    allocatedObjects = new vector<AbstractVMObject*>();
    allObjects.push_back(&allocatedObjects);
}

void MarkSweepHeap::UnregisterThread() {
    // TODO: move to global list, which is cleaned up on every GC, or so
}

AbstractVMObject* MarkSweepHeap::AllocateObject(size_t size) {
    AbstractVMObject* newObject = reinterpret_cast<AbstractVMObject*>(malloc(size));
    if (newObject == nullptr) {
        Universe::ErrorExit(("Failed to allocate " + to_string(size) + " Bytes.\n").c_str());
    }
    spcAlloc += size;
    memset((void*) newObject, 0, size);
    
    allocatedObjects->push_back(newObject);
    
    // let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit)
        triggerGC();
    return newObject;
}
