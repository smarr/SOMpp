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
    void Free(size_t);
    void RelocatePage();
    void ResetAmountOfLiveData();

    
    void SetNonRelocatablePage(PauselessPage* page) { nonRelocatablePage = page; }
    void SetInterpreter(Interpreter* interp) { interpreter = interp; }
    
    // These pages are allocated in aligned memory. The page is aligned with
    // its size, which should be a power of two, and thereby allow us to mask
    // out the address of a pointer in the page, and get to the page object.
    void* operator new(size_t count);

    
private:
    AbstractVMObject* allocate(size_t);
    AbstractVMObject* allocateNonRelocatable(size_t);
    
    bool isFull() { return nextFreePosition > treshold; }

    PauselessHeap* const heap;
    Interpreter* interpreter;

    void* treshold;
    
    void* const       bufferEnd;
    void*             nextFreePosition;

    PauselessPage* nonRelocatablePage;

    bool blocked;
    std::atomic<AbstractVMObject*>* sideArray;
    uintptr_t amountLiveData;
    
    void* buffer[];
};
