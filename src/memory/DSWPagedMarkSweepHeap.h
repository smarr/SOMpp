#pragma once
/*
 * The heap is divided into pages, and a page represents a single size class.
 * The maximum size class is MAX_SMALL_OBJECT_SIZE (2048 bytes).
 * Larger objects are allocated directly with malloc.
 *
 * Marking happens at a safe point, a time after a GC was triggered.
 * Sweeping is lazy, except for large objects.
 *
 * A collection itself only marks. Dead objects are reclaimed as part of
 * allocation when there's no free space in the size class.
 */
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../misc/defs.h"
#include "Heap.h"

class DSWPagedMarkSweepHeap : public Heap<DSWPagedMarkSweepHeap> {
    friend class DSWPagedMarkSweepCollector;
    struct FreeListEntry {
        FreeListEntry* next;
    };
    struct Page {
        char* memory;
        size_t sweptLastAtEpoch;
    };

public:
    explicit DSWPagedMarkSweepHeap(size_t objectSpaceSize);
    ~DSWPagedMarkSweepHeap();
    void* AllocateObject(size_t size);

    static constexpr size_t GC_IN_PROGRESS_BIT = ~(SIZE_MAX >> 1U);

private:
    static size_t sizeClassIndex(size_t size);
    void carveNewPage(size_t classIndex);
    bool sweepPageAt(size_t classIndex, size_t pageIndex);
    bool sweepNextPage(size_t classIndex);
    void* allocateLargeObject(size_t size);
    void accountAllocation(size_t bytes);

    std::vector<FreeListEntry*> freeLists;        // per size class
    std::vector<std::vector<Page*>> classPages;   // per size class
    std::vector<size_t> sweepCursor;              // per size class
    std::vector<AbstractVMObject*> largeObjects;  // not page-allocated

    size_t epoch{0};  // current live mark; bumped each collection
    size_t spcAlloc{0};
    size_t collectionLimit;
    // floor for collectionLimit (~the configured heap size): collect only once
    // about a heap's worth has been allocated, like the copying collector,
    // rather than every ~live-set bytes.
    size_t minCollectionLimit;
};
