#include "MarkSweepHeap.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../memory/Heap.h"
#include "../vm/Print.h"
#include "../vmobjects/AbstractObject.h"
#include "MarkSweepCollector.h"

MarkSweepHeap::MarkSweepHeap(size_t objectSpaceSize)
    : Heap<MarkSweepHeap>(new MarkSweepCollector(this)),
      allocatedObjects(new vector<AbstractVMObject*>()), spcAlloc(0),
      // our initial collection limit is 90% of objectSpaceSize
      collectionLimit((size_t)((double)objectSpaceSize * 0.9)) {}

AbstractVMObject* MarkSweepHeap::AllocateObject(size_t size) {
    auto* newObject = (AbstractVMObject*)malloc(size);
    if (newObject == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Quit(-1);
    }
    spcAlloc += size;
    memset((void*)newObject, 0, size);
    // AbstractObjects (Integer,...) have no Size field anymore -> set within
    // VMObject's new operator
    allocatedObjects->push_back(newObject);
    // let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit) {
        requestGC();
    }
    return newObject;
}
