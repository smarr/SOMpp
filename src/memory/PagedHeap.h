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

    void ReturnFullPage(PAGE_T* page) {
        lock_guard<mutex> lock(pages_mutex);
        ReturnFullPage_alreadyLocked(page);
    }
    
    void ReturnFullPage_alreadyLocked(PAGE_T* page) {
        fullPages.push_back(page);
    }
    
    void ReturnFreePage(PAGE_T* page) {
        lock_guard<mutex> lock(pages_mutex);
        ReturnFreePage_alreadyLocked(page);
    }

    void ReturnFreePage_alreadyLocked(PAGE_T* page) {
        freePages.push_back(page);
    }
    
    bool IsInFullPages(PAGE_T* page) {
        lock_guard<mutex> lock(pages_mutex);
        return IsInFullPages_alreadyLocked(page);
    }
    
    bool IsInFullPages_alreadyLocked(PAGE_T* page) {
        return std::find(fullPages.begin(), fullPages.end(), page) != fullPages.end();
    }
    
    vector<PAGE_T*>& GetUsedPages_alreadyLocked() {
        return usedPages;
    }
    
    vector<PAGE_T*>& GetFullPages_alreadyLocked() {
        return fullPages;
    }


    const size_t pageSize;
    
private:
    HEAP_T* const heap;
    
    const size_t maxNumPages;
    size_t currentNumPages;

    mutex pages_mutex;
    
    vector<PAGE_T*> usedPages;
    vector<PAGE_T*> freePages;
    vector<PAGE_T*> fullPages;  // currently only used by pauseless GC
};
