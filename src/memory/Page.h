//
//  Page.h
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#ifndef SOM_Page_h
#define SOM_Page_h

#include <cstdlib>
#include "PagedHeap.h"

class PauselessCollectorThread;
class Interpreter;

using namespace std;

class Page {
    
public:
    
    Page(void* pageStart, PagedHeap* heap);
    
    AbstractVMObject* AllocateObject(size_t size);
    bool Full();
    void ClearPage();
    
#if GC_TYPE==PAUSELESS
    void Block();
    void UnBlock();
    bool Blocked();
    AbstractVMObject* LookupNewAddress(AbstractVMObject*, Interpreter*);
    AbstractVMObject* LookupNewAddress(AbstractVMObject*, PauselessCollectorThread*);
    void AddAmountLiveData(size_t);
    double GetPercentageLiveData();
    void Free(size_t);
    void RelocatePage();
    void ResetAmountOfLiveData();
#endif
    
private:
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

#endif
