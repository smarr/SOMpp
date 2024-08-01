#pragma once

#include "../misc/defs.h"
#include "GarbageCollector.h"

class GenerationalHeap;
class GenerationalCollector : public GarbageCollector<GenerationalHeap> {
public:
    GenerationalCollector(GenerationalHeap* heap);
    void Collect() override;

private:
    uintptr_t majorCollectionThreshold;
    size_t matureObjectsSize;
    void MajorCollection();
    void MinorCollection();
};
