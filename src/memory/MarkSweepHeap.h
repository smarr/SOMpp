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
    
    static void free(void* ptr) {
        ::free(ptr);
    }
    
    mutex pages_mutex;
    unordered_set<MemoryPage<MarkSweepHeap>*> pages;
    vector<MemoryPage<MarkSweepHeap>*> yieldedPages;
    
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
        void* newObject = malloc(size);
        
        if (newObject == nullptr) {
            heap->FailedAllocation(size);
        }
        memset(newObject, 0, size);
        
        
        spaceAllocated += size;
        allocatedObjects->push_back(reinterpret_cast<AbstractVMObject*>(newObject));
        
        // let's see if we have to trigger the GC
        if (spaceAllocated >= heap->collectionLimit)
            heap->triggerGC();
        return newObject;
    }

private:
    vector<AbstractVMObject*>* allocatedObjects;
    size_t spaceAllocated;
    MarkSweepHeap* const heap;

    friend class MarkSweepHeap;
    friend class MarkSweepCollector;
};
