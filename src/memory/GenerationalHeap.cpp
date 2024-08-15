#include "GenerationalHeap.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../vm/Print.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMObjectBase.h"
#include "GenerationalCollector.h"
#include "Heap.h"

using namespace std;

GenerationalHeap::GenerationalHeap(size_t objectSpaceSize)
    : Heap<GenerationalHeap>(new GenerationalCollector(this)) {
    // our initial collection limit is 90% of objectSpaceSize
    // collectionLimit = objectSpaceSize * 0.9;

    nursery = malloc(objectSpaceSize);
    nurserySize = objectSpaceSize;
    maxNurseryObjSize = objectSpaceSize / 2;
    nursery_end = (size_t)nursery + nurserySize;
    matureObjectsSize = 0;
    memset(nursery, 0x0, objectSpaceSize);
    collectionLimit =
        (void*)((size_t)nursery + ((size_t)(objectSpaceSize * 0.9)));
    nextFreePosition = nursery;
    allocatedObjects = new vector<AbstractVMObject*>();
    oldObjsWithRefToYoungObjs = new vector<size_t>();
}

AbstractVMObject* GenerationalHeap::AllocateNurseryObject(size_t size) {
    auto* newObject = (AbstractVMObject*)nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if ((size_t)nextFreePosition > nursery_end) {
        ErrorPrint("\nFailed to allocate " + to_string(size) +
                   " Bytes in nursery.\n");
        Quit(-1);
    }
    // let's see if we have to trigger the GC
    if (nextFreePosition > collectionLimit) {
        requestGC();
    }
    return newObject;
}

AbstractVMObject* GenerationalHeap::AllocateMatureObject(size_t size) {
    auto* newObject = (AbstractVMObject*)malloc(size);
    if (newObject == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Quit(-1);
    }
    allocatedObjects->push_back(newObject);
    matureObjectsSize += size;
    return newObject;
}

void GenerationalHeap::writeBarrier_OldHolder(VMObjectBase* holder,
                                              const vm_oop_t referencedObject) {
    if (isObjectInNursery(referencedObject)) {
        oldObjsWithRefToYoungObjs->push_back((size_t)holder);
        holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
    }
}
