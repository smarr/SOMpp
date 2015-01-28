#pragma once

#include <cstdlib>
#include <misc/defs.h>

#include "Heap.h"

#include <mutex>
#include <unordered_set>
#include <vector>


class MarkSweepPage;

class MarkSweepHeap : public Heap<MarkSweepHeap> {
public:
    typedef MarkSweepPage MemoryPage;
    
    MarkSweepHeap(size_t pageSize, size_t objectSpaceSize);
    void* AllocateObject(size_t size);
    
    MarkSweepPage* RegisterThread();
    void UnregisterThread(MarkSweepPage* current);
    
private:
    size_t collectionLimit;
    
    static void free(void* ptr) {
        ::free(ptr);
    }
    
    mutex pages_mutex;
    unordered_set<MarkSweepPage*> pages;
    vector<MarkSweepPage*> yieldedPages;
    
    friend class MarkSweepPage;
    friend class MarkSweepCollector;
};
