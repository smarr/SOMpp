#pragma once

#include "../misc/defs.h"
#include "GarbageCollector.h"

/**
 * This collector marks the reachable objects using the Deutsch-Schorr-Waite
 * pointer-reversal algorithm instead of an explicit data structure.
 *
 * While tracing, the slot that leads from a parent to the object currently
 * being visited temporarily stores the pointer to the parent's parent.
 *
 * While an object's slots are visited, its gcField holds
 * `DSWPagedMarkSweepHeap::GC_IN_PROGRESS_BIT` and the index of the visited
 * slot.
 *
 * Once all slots are visited, marking is complete and all live objects
 * have the gcField set to the current epoch, which indicates them as marked.
 */
class DSWPagedMarkSweepHeap;
class DSWPagedMarkSweepCollector
    : public GarbageCollector<DSWPagedMarkSweepHeap> {
public:
    explicit DSWPagedMarkSweepCollector(DSWPagedMarkSweepHeap* heap)
        : GarbageCollector(heap) {}
    void Collect() override;

private:
    static void markReachableObjects();
};
