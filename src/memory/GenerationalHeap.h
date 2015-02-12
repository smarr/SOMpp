#pragma once

#include <assert.h>
#include <string.h>

#include <mutex>
#if UNITTESTS
  #include <set>
#endif
#include <vector>

#include <misc/defs.h>

#include "Heap.h"

#include <memory/PagedHeap.h>
#include <vmobjects/VMObjectBase.h>


#if UNITTESTS
struct VMObjectCompare {
    bool operator() (pair<vm_oop_t, vm_oop_t> lhs,
                     pair<vm_oop_t, vm_oop_t> rhs) const
    {   return (size_t) lhs.first < (size_t) rhs.first
            && (size_t) lhs.second < (size_t) rhs.second;
    }
};
#endif

class GenerationalHeap;
class NurseryPage;


// Nursary uses pages, each thread has at least one.
// This is the same approach as with the copying heap.
class GenerationalHeap : public Heap<GenerationalHeap> {
public:
    typedef NurseryPage MemoryPage;
    
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

    bool HasReachedMaxPageCount(size_t currentNumPages, size_t maxNumPages) {
        return currentNumPages > maxNumPages;
    }
    
    NurseryPage* RegisterThread();
    void UnregisterThread(NurseryPage*);


#if UNITTESTS
    std::set< pair<AbstractVMObject*, vm_oop_t>, VMObjectCompare > writeBarrierCalledOn;
#endif
    
private:
    PagedHeap<NurseryPage, GenerationalHeap> pagedHeap;
    
    mutex allocatedObjects_mutex;
    size_t sizeOfMatureObjectHeap;
    vector<AbstractVMObject*>* allocatedObjects;
    
    void writeBarrier_OldHolder(AbstractVMObject* holder, const vm_oop_t
                                referencedObject);
    
    inline NurseryPage* getPageFromObj(AbstractVMObject* obj) {
        uintptr_t bits = reinterpret_cast<uintptr_t>(obj);
        uintptr_t mask = ~(pagedHeap.pageSize - 1);
        uintptr_t page = bits & mask;
        return reinterpret_cast<NurseryPage*>(page);
    }
    
    friend class GenerationalCollector;
    friend NurseryPage;
};

