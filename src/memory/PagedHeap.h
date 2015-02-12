#pragma once

#include <mutex>
#include <vector>


template <class PAGE_T, class HEAP_T>
class PagedHeap {
public:
    PagedHeap(HEAP_T* heap, size_t pageSize, size_t maxNumPages)
      : heap(heap), pageSize(pageSize), maxNumPages(maxNumPages),
        currentNumPages(0) {}
    
    PAGE_T* GetNextPage() {
        lock_guard<mutex> lock(pages_mutex);
        return GetNextPage_alreadyLocked();
    }
    PAGE_T* GetNextPage_alreadyLocked() {
        PAGE_T* result;
        
        if (freePages.empty()) {
            currentNumPages++;
            if (heap->HasReachedMaxPageCount(currentNumPages, maxNumPages)) {
                heap->ReachedMaxNumberOfPages(); // won't return
                return nullptr;                  // is not executed!
            }
            
            result = new PAGE_T(heap);
        } else {
            result = freePages.back();
            freePages.pop_back();
        }
        
        usedPages.push_back(result);
        
        // let's see if we have to trigger the GC
        if (usedPages.size() > 0.9 * maxNumPages) {
            heap->TriggerGC();
        }
        
        return result;
    }
    
    const size_t pageSize;

# warning TODO: make private again, or protect the fields somehow
    vector<PAGE_T*> usedPages;
    vector<PAGE_T*> freePages;

    
private:
    HEAP_T* const heap;
    
    const size_t maxNumPages;
    size_t currentNumPages;

    mutex pages_mutex;
};