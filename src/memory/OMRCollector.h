#pragma once

#include "../misc/defs.h"

#include "GarbageCollector.h"

class OMRHeap;
class OMRCollector : public GarbageCollector<OMRHeap> {
public:
    OMRCollector(OMRHeap* heap) : GarbageCollector(heap) {
    }
    void Collect();
private:
};
