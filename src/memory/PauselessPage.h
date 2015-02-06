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
class PauselessHeap;
class PauselessCollectorThread;
class Interpreter;

using namespace std;

class PauselessPage {
    
public:
    
    PauselessPage(void* pageStart, PauselessHeap* heap);
    
    AbstractVMObject* AllocateObject(size_t size, bool nonRelocatable = false);
    void ClearPage();
    
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

    
    void SetNonRelocatablePage(PauselessPage* page) { nonRelocatablePage = page; }
    void SetInterpreter(Interpreter* interp) { interpreter = interp; }
    
private:
    AbstractVMObject* allocate(size_t);
    AbstractVMObject* allocateNonRelocatable(size_t);
    
    bool isFull() { return nextFreePosition > treshold; }
    
    Interpreter* interpreter;
    size_t pageStart;
    size_t pageEnd;
    void* treshold;
    void* volatile nextFreePosition;
    PauselessHeap* heap;
    
    PauselessPage* nonRelocatablePage;

    bool blocked;
    std::atomic<AbstractVMObject*>* sideArray;
    long amountLiveData;
};
