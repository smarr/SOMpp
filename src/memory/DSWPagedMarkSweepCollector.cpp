#include "DSWPagedMarkSweepCollector.h"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

#include "../memory/Heap.h"
#include "../misc/Timer.h"
#include "../misc/debug.h"
#include "../vm/IsValidObject.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/ObjectFormats.h"
#include "DSWPagedMarkSweepHeap.h"

static size_t epoch = 0;
static size_t markedBytes = 0;

void DSWPagedMarkSweepCollector::Collect() {
    DebugLog("DSWPagedMarkSweep Collect\n");

    auto* heap = GetHeap<DSWPagedMarkSweepHeap>();
    Timer::GCTimer.Resume();
    heap->resetGCTrigger();

    // Start a new cycle, epoch is only compared by identity
    // so wrap around is not an issue.
    heap->epoch += 1;
    epoch = heap->epoch;
    markedBytes = 0;

    // Drop all free lists. This guarantees that we don't allocate into pages
    // that still need to be swept, which is important because sweeping is lazy.
    for (auto& list : heap->freeLists) {
        list = nullptr;
    }
    for (auto& cursor : heap->sweepCursor) {
        cursor = 0;
    }

    markReachableObjects();

    // Sweep the large objects eagerly
    std::vector<AbstractVMObject*> survivingLarge;
    for (auto* obj : heap->largeObjects) {
        if (obj->GetGCField() == epoch) {
            survivingLarge.push_back(obj);
        } else {
            heap->FreeObject(obj);
        }
    }
    heap->largeObjects = std::move(survivingLarge);

    heap->spcAlloc = markedBytes;
    size_t const grown = 2 * markedBytes;
    heap->collectionLimit =
        grown > heap->minCollectionLimit ? grown : heap->minCollectionLimit;
    Timer::GCTimer.Halt();
}

static inline bool isMarked(AbstractVMObject* obj) {
    size_t const gcField = obj->GetGCField();
    return gcField == epoch ||
           (gcField & DSWPagedMarkSweepHeap::GC_IN_PROGRESS_BIT) != 0;
}

static inline size_t getSlotIndex(AbstractVMObject* obj) {
    return obj->GetGCField() & ~DSWPagedMarkSweepHeap::GC_IN_PROGRESS_BIT;
}

static inline void setSlotIndex(AbstractVMObject* obj, size_t index) {
    obj->SetGCField(index | DSWPagedMarkSweepHeap::GC_IN_PROGRESS_BIT);
}

static inline bool needsMarking(gc_oop_t oop) {
    return oop != nullptr && !IS_TAGGED(oop) && !isMarked(AS_OBJ(oop));
}

static inline void markObject(AbstractVMObject* obj) {
    assert(IsValidObject(obj));
    setSlotIndex(obj, 0);
    markedBytes += obj->GetObjectSize();
}

/**
 * Marks all objects reachable from `root` using Deutsch-Schorr-Waite.
 */
static gc_oop_t markFromRoot(gc_oop_t root) {
    # warning "TODO: Not yet implemented"
    return root;
}

void DSWPagedMarkSweepCollector::markReachableObjects() {
    Universe::WalkGlobals(markFromRoot);
}
