#pragma once

#include "../misc/defs.h"

#include "Heap.h"
#include "Page.h"

#include <mutex>
#include <unordered_set>


class MarkSweepHeap : public Heap<MarkSweepHeap> {
public:
    MarkSweepHeap(size_t objectSpaceSize = 1048576);
    void* AllocateObject(size_t size);
    
    Page* RegisterThread();
    void UnregisterThread(Page* current);
    
private:
    size_t collectionLimit;
    
    static void* allocate(size_t size, MemoryPage<MarkSweepHeap>* page ALLOC_OUTSIDE_NURSERY_DECL);
    static void free(void* ptr) {
        ::free(ptr);
    }
    
    mutex pages_mutex;
    unordered_set<Page*> pages;
    vector<Page*> yieldedPages;
    
    friend class MemoryPage<MarkSweepHeap>;
    friend class MarkSweepCollector;
};

template<>
class MemoryPage<MarkSweepHeap> {
public:
    MemoryPage<MarkSweepHeap>(MarkSweepHeap* heap) : heap(heap) {
        allocatedObjects = new vector<AbstractVMObject*>();
    }
    
    ~MemoryPage<MarkSweepHeap>() {
        delete allocatedObjects;
    }
    
    void* AllocateObject(size_t size ALLOC_OUTSIDE_NURSERY_DECL) {
        return MarkSweepHeap::allocate(size, this ALLOC_HINT);
    }

private:
    vector<AbstractVMObject*>* allocatedObjects;
    size_t spaceAllocated;
    MarkSweepHeap* const heap;

    friend class MarkSweepHeap;
    friend class MarkSweepCollector;
};
