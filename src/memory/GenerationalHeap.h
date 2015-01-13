#pragma once

#include <assert.h>

#include <misc/defs.h>

#include "Heap.h"

#include <vmobjects/VMObjectBase.h>
#include <vm/Universe.h>


#ifdef UNITTESTS
struct VMObjectCompare {
    bool operator() (pair<vm_oop_t, vm_oop_t> lhs,
                     pair<vm_oop_t, vm_oop_t> rhs) const
    {   return (size_t) lhs.first < (size_t) rhs.first
            && (size_t) lhs.second < (size_t) rhs.second;
    }
};
#endif

class GenerationalHeap;
typedef MemoryPage<GenerationalHeap> NurseryPage;


// Nursary uses pages, each thread has at least one.
// This is the same approach as with the copying heap.
class GenerationalHeap : public Heap<GenerationalHeap> {
public:
    GenerationalHeap(size_t pageSize, size_t objectSpaceSize);

    size_t GetMaxNurseryObjectSize();

    void writeBarrier(AbstractVMObject* holder, vm_oop_t referencedObject);
    
    void* AllocateMatureObject(size_t size) {
        void* newObject = (AbstractVMObject*) malloc(size);
        if (newObject == nullptr) {
            FailedAllocation(size);
            return nullptr; // never executed
        }
        
        {
            lock_guard<mutex> lock(allocatedObjects_mutex);
            allocatedObjects->push_back(reinterpret_cast<AbstractVMObject*>(newObject));
            sizeOfMatureObjectHeap += size;
        }
        return newObject;
    }

    
    Page* RegisterThread();
    void UnregisterThread(Page*);


#ifdef UNITTESTS
    std::set< pair<AbstractVMObject*, vm_oop_t>, VMObjectCompare > writeBarrierCalledOn;
#endif
    
private:
    const size_t pageSize;
    const size_t maxNumPages;
    size_t currentNumPages;
    
    mutex pages_mutex;
    vector<NurseryPage*> usedPages;
    vector<NurseryPage*> freePages;
    
    mutex allocatedObjects_mutex;
    size_t sizeOfMatureObjectHeap;
    vector<AbstractVMObject*>* allocatedObjects;
    
    NurseryPage* getNextPage() {
        lock_guard<mutex> lock(pages_mutex);
        return getNextPage_alreadyLocked();
    }
    
    NurseryPage* getNextPage_alreadyLocked();
    
    void writeBarrier_OldHolder(AbstractVMObject* holder, const vm_oop_t
                                referencedObject);
    
    inline NurseryPage* getPageFromObj(AbstractVMObject* obj) {
        uintptr_t bits = reinterpret_cast<uintptr_t>(obj);
        uintptr_t mask = ~(pageSize - 1);
        uintptr_t page = bits & mask;
        return reinterpret_cast<NurseryPage*>(page);
    }
    
    friend class GenerationalCollector;
    friend NurseryPage;
};


template<>
class MemoryPage<GenerationalHeap> {
public:
    
    MemoryPage<GenerationalHeap>(GenerationalHeap* heap)
        : heap(heap), interpreter(nullptr), next(nullptr),
          bufferEnd((void*)((uintptr_t)buffer + heap->pageSize - sizeof(MemoryPage<GenerationalHeap>))),
          nextFreePosition(buffer),
          maxObjectSize(heap->pageSize / 2) {
        memset(buffer, 0, heap->pageSize - sizeof(MemoryPage<GenerationalHeap>));
    }
    
    ~MemoryPage<GenerationalHeap>() {
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
    
    Page* GetCurrent();
    void SetInterpreter(Interpreter* interp) { interpreter = interp; }
    
    void WalkObjects(walk_heap_fn, Page* target);
    
    void Reset();
    
    
    // These pages are allocated in aligned memory. The page is aligned with
    // its size, which should be a power of two, and thereby allow us to mask
    // out the address of a pointer in the page, and get to the page object.
    void* operator new(size_t count) {
        assert(count == sizeof(MemoryPage<GenerationalHeap>));
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
    
    assert(Universe::IsValidObject(referencedObject));
    assert(Universe::IsValidObject((vm_oop_t) holder));

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
