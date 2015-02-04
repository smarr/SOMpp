#pragma once

#include <mutex>
#include <vector>

#include "Heap.h"

#include <misc/defs.h>

using namespace std;

class CopyingPage;

class CopyingHeap : public Heap<CopyingHeap> {
public:
    typedef CopyingPage MemoryPage;
    
    CopyingHeap(size_t pageSize, size_t maxHeapSize);
    
    Page* RegisterThread();
    void UnregisterThread(Page*);

private:
    const size_t pageSize;
    const size_t maxNumPages;
    size_t currentNumPages;
    
    mutex pages_mutex;
    vector<CopyingPage*> usedPages;
    vector<CopyingPage*> freePages;
    
    
    CopyingPage* getNextPage() {
        lock_guard<mutex> lock(pages_mutex);
        return getNextPage_alreadyLocked();
    }
    CopyingPage* getNextPage_alreadyLocked();

    friend class CopyingCollector;
    friend CopyingPage;
};
