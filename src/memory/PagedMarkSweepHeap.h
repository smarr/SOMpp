#pragma once

#include <cstddef>
#include <vector>

#include "../misc/defs.h"
#include "Heap.h"

class PagedMarkSweepHeap : public Heap<PagedMarkSweepHeap> {
    friend class PagedMarkSweepCollector;
    struct FreeListEntry {
        FreeListEntry* next;
    };
    // NOLINTNEXTLINE(altera-struct-pack-align): FPGA-specific, not relevant
    struct Page {
        char* memory;
        size_t sweptEpoch;  // last epoch this page was swept in
    };

public:
    explicit PagedMarkSweepHeap(size_t objectSpaceSize);
    ~PagedMarkSweepHeap();
    void* AllocateObject(size_t size);

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
