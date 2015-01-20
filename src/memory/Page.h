#pragma once
//
//  Page.h
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#include <cstdlib>
#include <atomic>

#include <misc/defs.h>

class AbstractVMObject;
class PagedHeap;
class PauselessCollectorThread;
class Interpreter;

using namespace std;

class Page {
    
public:
    
    Page(void* pageStart, PagedHeap* heap);
    
    AbstractVMObject* AllocateObject(size_t size ALLOC_OUTSIDE_NURSERY_DECL ALLOC_NON_RELOCATABLE_DECL);
    void ClearPage();
    
#if GC_TYPE==PAUSELESS
    void Block();
    void UnBlock();
    FORCE_INLINE bool Blocked() {
        return blocked;
    }
    AbstractVMObject* LookupNewAddress(AbstractVMObject*, Interpreter*);
    AbstractVMObject* LookupNewAddress(AbstractVMObject*, PauselessCollectorThread*);
    void AddAmountLiveData(size_t);
    double GetPercentageLiveData();
    void Free(size_t);
    void RelocatePage();
    void ResetAmountOfLiveData();
#endif
    
private:
    bool isFull() { return nextFreePosition > treshold; }
    
    size_t pageStart;
    size_t pageEnd;
    void* treshold;
    void* volatile nextFreePosition;
    PagedHeap* heap;
        
#if GC_TYPE==PAUSELESS
    bool blocked;
    std::atomic<AbstractVMObject*>* sideArray;
    long amountLiveData;
#endif
    
};
