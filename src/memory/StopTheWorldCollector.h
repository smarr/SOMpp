#pragma once
#include <misc/defs.h>

#if GC_TYPE != PAUSELESS

#include "GarbageCollector.h"

class StopTheWorldCollector : public GarbageCollector {
    
public:
    StopTheWorldCollector(PagedHeap* h) : GarbageCollector(h) {};
    virtual void Collect() = 0;
    
};

#endif
