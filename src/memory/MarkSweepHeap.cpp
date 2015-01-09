#include <string.h>

#include "MarkSweepCollector.h"
#include "MarkSweepHeap.h"

#include <vm/Universe.h>
#include <vmobjects/AbstractObject.h>



MarkSweepHeap::MarkSweepHeap(size_t objectSpaceSize)
    : Heap<MarkSweepHeap>(new MarkSweepCollector(this), objectSpaceSize),
        // our initial collection limit is 90% of objectSpaceSize
        collectionLimit(objectSpaceSize * 0.9) { }

Page* MarkSweepHeap::RegisterThread() {
    lock_guard<mutex> lock(pages_mutex);
    
    Page* page = new Page(this);
    pages.insert(page);
    
    return page;
}

void MarkSweepHeap::UnregisterThread(Page* page) {
    lock_guard<mutex> lock(pages_mutex);
    
    pages.erase(page);
    yieldedPages.push_back(page);
}

void* MarkSweepHeap::allocate(size_t size, MemoryPage<MarkSweepHeap>* page ALLOC_OUTSIDE_NURSERY_DECL) {
    void* newObject = malloc(size);
    
    if (newObject == nullptr) {
        Universe::ErrorExit(("Failed to allocate " + to_string(size) + " Bytes.\n").c_str());
    }
    memset(newObject, 0, size);
    
    
    page->spaceAllocated += size;
    page->allocatedObjects->push_back(reinterpret_cast<AbstractVMObject*>(newObject));
    
    // let's see if we have to trigger the GC
    if (page->spaceAllocated >= page->heap->collectionLimit)
        page->heap->triggerGC();
    return newObject;
}
