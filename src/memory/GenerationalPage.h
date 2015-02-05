#pragma once

#include <vector>

#include <memory/GenerationalHeap.h>

class Interpreter;

class NurseryPage {
public:
    
    NurseryPage(GenerationalHeap* heap)
    : heap(heap), interpreter(nullptr), next(nullptr),
      bufferEnd((void*)((uintptr_t)buffer + heap->pageSize - sizeof(NurseryPage))),
      nextFreePosition(buffer),
      maxObjectSize(heap->pageSize / 2) {
        memset(buffer, 0, heap->pageSize - sizeof(NurseryPage));
    }
    
    ~NurseryPage() {
        free(buffer);
    }
    
    void* AllocateObject(size_t size, bool outsideNursery = false) {
        if (outsideNursery) {
            return heap->AllocateMatureObject(size);
        }
        
        void* newObject = nextFreePosition;
        void* newFreePosition = (void*)((uintptr_t)nextFreePosition + size);
        
        if (newFreePosition > bufferEnd) {
            return allocateInNextPage(size ALLOC_HINT);
        }
        
        nextFreePosition = newFreePosition;
        return newObject;
    }
    
    inline size_t GetMaxObjectSize() { return maxObjectSize; }
    
    NurseryPage* GetCurrent();
    void SetInterpreter(Interpreter* interp) { interpreter = interp; }
    
    void WalkObjects(walk_heap_fn, Page* target);
    
    void Reset();
    
    
    // These pages are allocated in aligned memory. The page is aligned with
    // its size, which should be a power of two, and thereby allow us to mask
    // out the address of a pointer in the page, and get to the page object.
    void* operator new(size_t count) {
        assert(count == sizeof(NurseryPage));
        size_t size = GetHeap<GenerationalHeap>()->pageSize;
        assert(size > count);
        
        void* result;
        int r = posix_memalign(&result, size, size);
        assert(r == 0);
        
        return result;
    }
    
    
private:
    GenerationalHeap* const heap;
    Interpreter*      interpreter;
    NurseryPage*      next;
    
    void* const       bufferEnd;
    
    void*             nextFreePosition;
    
    const size_t      maxObjectSize;
    
    vector<AbstractVMObject*> oldObjsWithRefToYoungObjs;
    
    void*             buffer[];
    
    void* allocateInNextPage(size_t size ALLOC_OUTSIDE_NURSERY_DECL);
    
    friend class GenerationalHeap;
    friend class GenerationalCollector;
};

inline void GenerationalHeap::writeBarrier(AbstractVMObject* holder,
                                           vm_oop_t referencedObject) {
#ifdef UNITTESTS
    writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif
    
//    assert(Universe::IsValidObject(referencedObject));
//    assert(Universe::IsValidObject((vm_oop_t) holder));
    
    if (IS_TAGGED(referencedObject)) {
        return;
    }
    
    // TODO: can we sort out header include order, and avoid the hack?
    //    intptr_t gcField = holder->GetGCField();
    intptr_t gcField = *(((intptr_t*)holder)+1);
    
    if ((gcField & (MASK_OBJECT_IS_OLD | MASK_SEEN_BY_WRITE_BARRIER)) ==  MASK_OBJECT_IS_OLD) {
        writeBarrier_OldHolder(holder, referencedObject);
    }
}