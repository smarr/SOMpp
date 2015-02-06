#pragma once


class Interpreter;

class CopyingPage {
public:
    CopyingPage(CopyingHeap* heap)
    : heap(heap), interpreter(nullptr), next(nullptr),
    buffer(malloc(heap->pageSize)),
    bufferEnd((void*)((uintptr_t)buffer + heap->pageSize)),
    nextFreePosition(buffer) {
        memset(buffer, 0, heap->pageSize);
    }
    
    ~CopyingPage() {
        free(buffer);
    }
    
    void* AllocateObject(size_t size ALLOC_OUTSIDE_NURSERY_DECL) {
        assert(interpreter);
        
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
    // when a page fills up, the interpreter can request a new page.
    // we stop using the old page when it is full.
    
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
    
    void* allocateInNextPage(size_t size ALLOC_OUTSIDE_NURSERY_DECL);
    
    friend class CopyingHeap;
    friend class CopyingCollector;
};
