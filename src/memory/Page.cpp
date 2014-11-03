//
//  Page.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#include "Page.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"
#include "../interpreter/Interpreter.h"
#include <atomic>

Page::Page(void* pageStart, PagedHeap* heap) {
    this->heap = heap;
    this->nextFreePosition = pageStart;
    this->pageStart = (size_t)pageStart;
    this->pageEnd = this->pageStart + heap->pageSize;
    collectionLimit = (void*)((size_t)pageStart + ((size_t)(heap->pageSize * 0.9)));
}

AbstractVMObject* Page::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    
    void* test = (void*)pageEnd;
    
    if ((size_t)nextFreePosition > pageEnd) {
        cout << "Failed to allocate " << size << " Bytes in page." << endl;
        _UNIVERSE->Quit(-1);
    }
    return newObject;
}

bool Page::Full() {
    return nextFreePosition > collectionLimit;
}

void Page::ClearPage() {
    nextFreePosition = (void*) pageStart;
}

#if GC_TYPE==PAUSELESS
void Page::Block() {
    blocked = true;
    sideArray = new std::atomic<AbstractVMObject*>[PAGE_SIZE / 8];
    for (int i=0; i < (PAGE_SIZE / 8); i++) {
        sideArray[i] = nullptr;
    }
}

void Page::UnBlock() {
    assert(blocked == true);
    blocked = false;
    delete [] sideArray;
}

bool Page::Blocked() {
    return blocked;
}

AbstractVMObject* Page::LookupNewAddress(AbstractVMObject* oldAddress, Interpreter* thread) {
    long position = ((size_t)oldAddress - pageStart)/8;
    if (!sideArray[position]) {
        AbstractVMObject* newLocation = oldAddress->Clone(thread);
        AbstractVMObject* test = nullptr;
        if (!sideArray[position].compare_exchange_strong(test, newLocation)) {
            _UNIVERSE->GetInterpreter()->GetPage()->Free(newLocation->GetObjectSize());
        }
    }
    return static_cast<AbstractVMObject*>(sideArray[position]);
}

AbstractVMObject* Page::LookupNewAddress(AbstractVMObject* oldAddress, PauselessCollectorThread* thread) {
    long position = ((size_t)oldAddress - pageStart)/8;
    if (!sideArray[position]) {
        AbstractVMObject* newLocation = oldAddress->Clone(thread);
        AbstractVMObject* test = nullptr;
        if (!sideArray[position].compare_exchange_strong(test, newLocation)) {
            _UNIVERSE->GetInterpreter()->GetPage()->Free(newLocation->GetObjectSize());
        }
    }
    return static_cast<AbstractVMObject*>(sideArray[position]);
}

void Page::AddAmountLiveData(size_t objectSize) {
    amountLiveData += objectSize;
}

long Page::GetAmountOfLiveData() {
    return amountLiveData;
}

void Page::Free(size_t numBytes) {
    nextFreePosition = (void*)((size_t)nextFreePosition - numBytes);
}

void Page::RelocatePage() {
    for (AbstractVMObject* currentObject = (AbstractVMObject*) pageStart;
         (size_t) currentObject < (size_t) nextFreePosition;
         currentObject = (AbstractVMObject*) (currentObject->GetObjectSize() + (size_t) currentObject)) {
        //size_t pageNumber = ((size_t)currentObject - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
        //assert(pageNumber < 1280);
        if (currentObject->GetGCField()) {
            AbstractVMObject* newLocation = currentObject->Clone(_HEAP->GetGCThread());
            //size_t pageNumber2 = ((size_t)newLocation - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
            //assert(pageNumber2 < 1280);
            long positionSideArray = ((size_t)currentObject - pageStart)/8;
            AbstractVMObject* test = nullptr;
            if (!sideArray[positionSideArray].compare_exchange_strong(test, newLocation)) {
                _HEAP->GetGCThread()->GetPage()->Free(currentObject->GetObjectSize());
            }
        }
    }
    amountLiveData = 0;
    nextFreePosition = (void*)pageStart;
}

#endif















/*
void Page::ClearMarkBits() {
    for (AbstractVMObject* currentObject = (AbstractVMObject*) pageStart;
         (size_t) currentObject < (size_t) nextFreePosition;
         currentObject = (AbstractVMObject*) (currentObject->GetObjectSize() + (size_t) currentObject)) {
        currentObject->SetGCField(0);
    }
}
 
 
 void test() {
 std::atomic<int>  ai;
 int  tst_val= 4;
 int  new_val= 5;
 ai= 3;
 
 // tst_val != ai   ==>  tst_val is modified
 ai.compare_exchange_strong( tst_val, new_val );
 }
*/