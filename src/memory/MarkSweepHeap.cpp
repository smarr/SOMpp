#include <string.h>

#include <memory/MarkSweepCollector.h>
#include <memory/MarkSweepHeap.h>
#include <memory/MarkSweepPage.h>

#include <vm/Universe.h>
#include <vmobjects/AbstractObject.h>



MarkSweepHeap::MarkSweepHeap(size_t pageSize, size_t objectSpaceSize)
    : Heap<MarkSweepHeap>(new MarkSweepCollector(this)),
        // our initial collection limit is 90% of objectSpaceSize
        collectionLimit(objectSpaceSize * 0.9) { }

MarkSweepPage* MarkSweepHeap::RegisterThread() {
    lock_guard<mutex> lock(pages_mutex);
    
    MarkSweepPage* page = new MarkSweepPage(this);
    pages.insert(page);
    
    return page;
}

void MarkSweepHeap::UnregisterThread(MarkSweepPage* page) {
    lock_guard<mutex> lock(pages_mutex);
    pages.erase(page);
    yieldedPages.push_back(page);
}

