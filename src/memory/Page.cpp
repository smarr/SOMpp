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
#include <misc/debug.h>

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
    if ((size_t)nextFreePosition > pageEnd) {
        sync_out(ostringstream() << "Failed to allocate " << size << " Bytes in page.");
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
    sideArray = new std::atomic<AbstractVMObject*>[heap->pageSize / 8];
    for (int i=0; i < (heap->pageSize / 8); i++) {
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
            //_UNIVERSE->GetInterpreter()->GetPage()->Free(newLocation->GetObjectSize());
        }
    }
    assert(Universe::IsValidObject((AbstractVMObject*) sideArray[position]));
    return static_cast<AbstractVMObject*>(sideArray[position]);
}

AbstractVMObject* Page::LookupNewAddress(AbstractVMObject* oldAddress, PauselessCollectorThread* thread) {
    long position = ((size_t)oldAddress - pageStart)/8;
    if (!sideArray[position]) {
        AbstractVMObject* newLocation = oldAddress->Clone(thread);
        AbstractVMObject* test = nullptr;
        if (!sideArray[position].compare_exchange_strong(test, newLocation)) {
            //_UNIVERSE->GetInterpreter()->GetPage()->Free(newLocation->GetObjectSize());
        }
    }
    return static_cast<AbstractVMObject*>(sideArray[position]);
}

void Page::AddAmountLiveData(size_t objectSize) {
    //pthread_mutex_lock
    amountLiveData += objectSize;
    //pthread_mutex_unlock
}

long Page::GetAmountOfLiveData() {
    return amountLiveData;
}

void Page::Free(size_t numBytes) {
    nextFreePosition = (void*)((size_t)nextFreePosition - numBytes);
}

void Page::RelocatePage() {
    AbstractVMObject* prevObject = NULL;
    for (AbstractVMObject* currentObject = (AbstractVMObject*) pageStart;
         (size_t) currentObject < (size_t) nextFreePosition;
         currentObject = (AbstractVMObject*) (currentObject->GetObjectSize() + (size_t) currentObject)) {
        //size_t pageNumber = ((size_t)currentObject - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
        //assert(pageNumber < 1280);
        assert(Universe::IsValidObject(currentObject));
        if (currentObject->GetGCField()) {
            AbstractVMObject* newLocation = currentObject->Clone(_HEAP->GetGCThread());
            //size_t pageNumber2 = ((size_t)newLocation - (size_t)(_HEAP->GetMemoryStart())) / _HEAP->GetPageSize();
            //assert(pageNumber2 < 1280);
            long positionSideArray = ((size_t)currentObject - pageStart)/8;
            AbstractVMObject* test = nullptr;
            pthread_mutex_lock(_HEAP->GetCompareAndSwapTestMutex());
            if (!sideArray[positionSideArray].compare_exchange_strong(test, newLocation)) {
                //_HEAP->GetGCThread()->GetPage()->Free(currentObject->GetObjectSize());
            }
            pthread_mutex_unlock(_HEAP->GetCompareAndSwapTestMutex());
            assert(Universe::IsValidObject((AbstractVMObject*) sideArray[positionSideArray]));
        }
        //prevObject = currentObject;
    }
    amountLiveData = 0;
    nextFreePosition = (void*)pageStart;
}

#endif
