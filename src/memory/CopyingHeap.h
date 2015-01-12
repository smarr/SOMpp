#pragma once

#include <string.h>
#include <mutex>
#include <unordered_set>

#include "Heap.h"

using namespace std;

class CopyingHeap;
typedef MemoryPage<CopyingHeap> CopyingPage;

class CopyingHeap : public Heap<CopyingHeap> {

public:
    CopyingHeap(size_t pageSize, size_t maxHeapSize);
    
    AbstractVMObject* AllocateObject(size_t size);
    
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

class Interpreter;

template<>
class MemoryPage<CopyingHeap> {
public:
    MemoryPage<CopyingHeap>(CopyingHeap* heap)
        : heap(heap), interpreter(nullptr), next(nullptr),
          buffer(malloc(heap->pageSize)),
          bufferEnd((void*)((uintptr_t)buffer + heap->pageSize)),
          nextFreePosition(buffer) {
        memset(buffer, 0, heap->pageSize);
    }
    
    ~MemoryPage<CopyingHeap>() {
        free(buffer);
    }
    
    void* AllocateObject(size_t size ALLOC_OUTSIDE_NURSERY_DECL) {
        void* newObject = nextFreePosition;
        void* newFreePosition = (void*)((uintptr_t)nextFreePosition + size);
        
        if (newFreePosition > bufferEnd) {
            return allocateInNextPage(size ALLOC_HINT);
        }

        nextFreePosition = newFreePosition;
        return newObject;
    }

    // return the current page to be used for allocation
    // this is managed by the interpreter associated with a page
    // when a page fills up, the interpreter can request a new page
    // we stop using the old page when it is full
    // and full means, something like failing
    // on an allocation reqest for an object that
    // is smaller than 2*sizeof(VMObject)

    Page* GetCurrent();
    void SetInterpreter(Interpreter* interp) { interpreter = interp; }
    
    void WalkObjects(walk_heap_fn, Page* target);

    void Reset();
    
private:

    CopyingHeap* const heap;
    Interpreter*       interpreter;
    CopyingPage*       next;

    void* const        buffer;
    void* const        bufferEnd;

    void*              nextFreePosition;

    bool isFull();
    void* allocateInNextPage(size_t size ALLOC_OUTSIDE_NURSERY_DECL);
    
    friend class CopyingHeap;
    friend class CopyingCollector;
};
