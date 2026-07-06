#pragma once

#include "../misc/defs.h"
#include "GarbageCollector.h"

class PagedMarkSweepHeap;
class PagedMarkSweepCollector : public GarbageCollector<PagedMarkSweepHeap> {
public:
    explicit PagedMarkSweepCollector(PagedMarkSweepHeap* heap)
        : GarbageCollector(heap) {}
    void Collect() override;

private:
    static void markReachableObjects();
};
