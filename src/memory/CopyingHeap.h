#pragma once

#include <mutex>
#include <vector>

#include "Heap.h"

#include <misc/defs.h>
#include <memory/PagedHeap.h>


using namespace std;

class CopyingPage;

class CopyingHeap : public Heap<CopyingHeap> {
public:
    typedef CopyingPage MemoryPage;
    
    CopyingHeap(size_t pageSize, size_t maxHeapSize);
    
    Page* RegisterThread();
    void UnregisterThread(Page*);
    
    bool HasReachedMaxPageCount(size_t currentNumPages, size_t maxNumPages) {
        // during the copy phase, we need twice as many pages, i.e.,
        // a heap twice as large. This is similar to the classic
        // semi-space copy-collector design. Except, that we allow
        // too much allocation.
        return currentNumPages > maxNumPages * 2;
    }

private:
    PagedHeap<CopyingPage, CopyingHeap> pagedHeap;
    

    friend class CopyingCollector;
    friend CopyingPage;
};
