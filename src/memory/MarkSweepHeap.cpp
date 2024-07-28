#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../memory/Heap.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "MarkSweepCollector.h"
#include "MarkSweepHeap.h"


MarkSweepHeap::MarkSweepHeap(size_t objectSpaceSize) : Heap<MarkSweepHeap>(new MarkSweepCollector(this)) {
    //our initial collection limit is 90% of objectSpaceSize
    collectionLimit = objectSpaceSize * 0.9;
    spcAlloc = 0;
    allocatedObjects = new vector<AbstractVMObject*>();
}

AbstractVMObject* MarkSweepHeap::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) malloc(size);
    if (newObject == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Universe::Quit(-1);
    }
    spcAlloc += size;
    memset((void*) newObject, 0, size);
    //AbstractObjects (Integer,...) have no Size field anymore -> set within VMObject's new operator
    allocatedObjects->push_back(newObject);
    //let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit) {
        requestGC();
    }
    return newObject;
}
