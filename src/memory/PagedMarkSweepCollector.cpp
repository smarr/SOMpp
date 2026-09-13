// A mark-and-sweep garbage collector. When memory runs low it finds every
// object the program can still reach ("mark"), and treats everything else as
// garbage whose memory can be reused ("sweep"). Objects are never moved.
//
// Marking starts from the roots - the globals and the interpreter's stack
// and follows references outward until the whole set of reachable objects has
// been visited.
//
// Sweeping is lazy. A collection itself only marks, which keeps the process
// pause short; reclaiming dead objects happens as the the program allocates.
// Reclaimed memory is recycled for new objects of the same size.
#include "PagedMarkSweepCollector.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "../memory/Heap.h"
#include "../misc/debug.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "PagedMarkSweepHeap.h"

size_t PagedMarkSweepCollector::epoch = 0;
size_t PagedMarkSweepCollector::markedBytes = 0;
std::vector<AbstractVMObject*> PagedMarkSweepCollector::markStack;

void PagedMarkSweepCollector::Collect() {
    DebugLog("PagedMarkSweep Collect\n");

    auto* heap = GetHeap<PagedMarkSweepHeap>();
    Timer::GCTimer.Resume();
    heap->resetGCTrigger();

    // New cycle. The epoch only increases, so a survivor marked in an older
    // cycle is never mistaken for live now -- no mark reset needed.
    heap->epoch += 1;
    epoch = heap->epoch;
    markedBytes = 0;

    // Drop all free lists (re-harvested by sweeping). This is what guarantees
    // no allocation into a not-yet-swept page, so any cell without the current
    // epoch found while sweeping is reclaimable.
    for (auto& list : heap->freeLists) {
        list = nullptr;
    }
    for (auto& cursor : heap->sweepCursor) {
        cursor = 0;
    }

    markReachableObjects();

    // Sweep the large-object space eagerly (few objects, so cheap).
    std::vector<AbstractVMObject*> survivingLarge;
    for (auto* obj : heap->largeObjects) {
        if (obj->GetGCField() == epoch) {
            survivingLarge.push_back(obj);
        } else {
            heap->FreeObject(obj);
        }
    }
    heap->largeObjects = std::move(survivingLarge);

    // Small dead objects (and empty pages) are reclaimed lazily during
    // allocation, not here, keeping this pause to just the mark phase.

    heap->spcAlloc = markedBytes;
    // Collect again after allocating ~max(live, a heap's worth). The floor
    // makes the heap size (-H / objectSpaceSize) actually govern GC frequency,
    // like the copying collector, instead of collecting every ~live bytes.
    size_t const grown = 2 * markedBytes;
    heap->collectionLimit =
        grown > heap->minCollectionLimit ? grown : heap->minCollectionLimit;
    Timer::GCTimer.Halt();
}

// Marks an object with the current epoch and queues it. Iterative (worklist),
// not recursive, so deep object graphs can't overflow the native stack.
static gc_oop_t mark_object(gc_oop_t oop) {
    if (IS_TAGGED(oop)) {
        return oop;
    }

    AbstractVMObject* obj = AS_OBJ(oop);

    if (obj->GetGCField() == PagedMarkSweepCollector::epoch) {
        return oop;
    }

    obj->SetGCField(PagedMarkSweepCollector::epoch);
    PagedMarkSweepCollector::markedBytes += obj->GetObjectSize();
    PagedMarkSweepCollector::markStack.push_back(obj);
    return oop;
}

void PagedMarkSweepCollector::markReachableObjects() {
    markStack.clear();
    Universe::WalkGlobals(mark_object);

    while (!markStack.empty()) {
        AbstractVMObject* obj = markStack.back();
        markStack.pop_back();
        obj->WalkObjects(mark_object);
    }
}
