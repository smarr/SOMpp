#pragma once

#include <misc/defs.h>

#include "GarbageCollector.h"

class GenerationalHeap;
class GenerationalCollector : public GarbageCollector<GenerationalHeap> {
public:
    GenerationalCollector(GenerationalHeap* heap);
    virtual ~GenerationalCollector() {}
    
    virtual void Collect();

private:
    size_t majorCollectionThreshold;

    void MajorCollection();
    void MinorCollection();
};
