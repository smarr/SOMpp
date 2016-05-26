#pragma once

#include "../misc/defs.h"

#include "GarbageCollector.h"

#if GC_TYPE == OMR_GARBAGE_COLLECTION

class OMRHeap;
class OMRCollector : public GarbageCollector<OMRHeap> {
public:
    OMRCollector(OMRHeap* heap) : GarbageCollector(heap) {
    }
    void Collect();
private:
};

#endif
