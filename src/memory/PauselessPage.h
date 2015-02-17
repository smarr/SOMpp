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
#include <memory/BaseThread.h>

class AbstractVMObject;
class PauselessHeap;
class PauselessCollectorThread;
class Interpreter;

using namespace std;

class PauselessPage {
    
public:
    
    PauselessPage(PauselessHeap* heap);
    
    AbstractVMObject* AllocateObject(size_t size, bool nonRelocatable = false);
    void ClearPage();
    
    void Block();
    void UnBlock();
    FORCE_INLINE bool Blocked() {
        return blocked;
    }
    AbstractVMObject* LookupNewAddress(AbstractVMObject*, BaseThread*);
    void AddAmountLiveData(size_t);
    double GetPercentageLiveData();
    void RelocatePage();
    void ResetAmountOfLiveData();

    
    void SetNonRelocatablePage(PauselessPage* page) { nonRelocatablePage = page; }
    void SetInterpreter(Interpreter* interp) { interpreter = interp; }
    
    // These pages are allocated in aligned memory. The page is aligned with
    // its size, which should be a power of two, and thereby allow us to mask
    // out the address of a pointer in the page, and get to the page object.
    void* operator new(size_t count);

    bool IsRelocated_racy(AbstractVMObject* obj) {
        uintptr_t position = ((uintptr_t)obj - (uintptr_t)buffer)/8;
        return sideArray[position];
    }
    
    bool CanAllocateAndIsNotReturned() { return canAllocateAndIsNotReturned; }
    
private:
    AbstractVMObject* allocate(size_t);
    AbstractVMObject* allocateNonRelocatable(size_t);
    
    void relocateObject(BaseThread*, AbstractVMObject*, uintptr_t);

    PauselessPage* getNextPage(bool nonRelocatable);
    
    bool isFull() { return nextFreePosition > treshold; }

    PauselessHeap* const heap;
    Interpreter* interpreter;

    void* treshold;
    
    void* const       bufferEnd;
    void*             nextFreePosition;
    
    // whether allocation is legal, and whether the page has not been returned
    // THREAD-LOCAL: assumed to be accessed only from the owning thread
    bool canAllocateAndIsNotReturned;

    PauselessPage* nonRelocatablePage;

    bool blocked;
    std::atomic<AbstractVMObject*>* sideArray;
    uintptr_t amountLiveData;
    
    void* buffer[];
};
