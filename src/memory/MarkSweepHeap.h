#pragma once

#include "../misc/defs.h"
#include "Heap.h"

class MarkSweepHeap : public Heap<MarkSweepHeap> {
    friend class MarkSweepCollector;

public:
    explicit MarkSweepHeap(size_t objectSpaceSize);
    AbstractVMObject* AllocateObject(size_t size);

private:
    vector<AbstractVMObject*>* allocatedObjects;
    size_t spcAlloc{0};
    size_t collectionLimit;
};
