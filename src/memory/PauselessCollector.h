#pragma once
#include "../misc/defs.h"
#if GC_TYPE == PAUSELESS

#include "GarbageCollector.h"

class PauselessCollector : public GarbageCollector {
public:
    PauselessCollector(PagedHeap* heap) : GarbageCollector(heap) {};
    void Collect();
private:
    bool phase;
};

#endif


/*
 intptr_t majorCollectionThreshold;
 size_t matureObjectsSize;
 void MajorCollection();
 void MinorCollection();
 void CopyInterpretersFrameAndThread();
*/