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
    this->pageEnd = this->pageStart + PAGE_SIZE;
    treshold = (void*)((size_t)pageStart + ((size_t)(PAGE_SIZE * 0.9)));
}

AbstractVMObject* Page::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
#if GC_TYPE==PAUSELESS
    if ((size_t)nextFreePosition > pageEnd) {
        sync_out(ostringstream() << "Failed to allocate " << size << " Bytes in page.");
        _UNIVERSE->Quit(-1);
    }
#elif GC_TYPE==GENERATIONAL
    if (nextFreePosition > treshold) {
        heap->RelinquishPage(this);
        _UNIVERSE->GetInterpreter()->SetPage(_HEAP->RequestPage());
    }
#endif
    return newObject;
}

bool Page::Full() {
    return nextFreePosition > treshold;
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
    memset((void*)pageStart, 0xa, PAGE_SIZE);
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
        void* oldPosition = thread->GetPage()->nextFreePosition;
        if (!sideArray[position].compare_exchange_strong(test, newLocation)) {
            thread->GetPage()->nextFreePosition = oldPosition;
        }
    }
    assert(Universe::IsValidObject((AbstractVMObject*) sideArray[position]));
    return sideArray[position];
}

AbstractVMObject* Page::LookupNewAddress(AbstractVMObject* oldAddress, PauselessCollectorThread* thread) {
    long position = ((size_t)oldAddress - pageStart)/8;
    if (!sideArray[position]) {
        AbstractVMObject* newLocation = oldAddress->Clone(thread);
        AbstractVMObject* test = nullptr;
        void* oldPosition = thread->GetPage()->nextFreePosition;
        if (!sideArray[position].compare_exchange_strong(test, newLocation)) {
            thread->GetPage()->nextFreePosition = oldPosition;
        }
    }
    assert(Universe::IsValidObject((AbstractVMObject*) sideArray[position]));
    return sideArray[position];
}

void Page::AddAmountLiveData(size_t objectSize) {
    //pthread_mutex_lock
    amountLiveData += objectSize;
    //pthread_mutex_unlock
}

double Page::GetPercentageLiveData() {
    return amountLiveData / PAGE_SIZE;
}

void Page::ResetAmountOfLiveData() {
    amountLiveData = 0;
}

void Page::Free(size_t numBytes) {
    nextFreePosition = (void*)((size_t)nextFreePosition - numBytes);
}

void Page::RelocatePage() {
    PauselessCollectorThread* thread = _HEAP->GetGCThread();
    for (AbstractVMObject* currentObject = (AbstractVMObject*) pageStart;
         (size_t) currentObject < (size_t) nextFreePosition;
         currentObject = (AbstractVMObject*) (currentObject->GetObjectSize() + (size_t) currentObject)) {
        assert(Universe::IsValidObject(currentObject));
        if (currentObject->GetGCField() == _HEAP->GetMarkValue()) {
            AbstractVMObject* newLocation = currentObject->Clone(_HEAP->GetGCThread());
            long positionSideArray = ((size_t)currentObject - pageStart)/8;
            AbstractVMObject* test = nullptr;
            void* oldPosition = thread->GetPage()->nextFreePosition;
            if (!sideArray[positionSideArray].compare_exchange_strong(test, newLocation)) {
                thread->GetPage()->nextFreePosition = oldPosition;
            }
            assert(Universe::IsValidObject((AbstractVMObject*) sideArray[positionSideArray]));
        }
    }
    amountLiveData = 0;
    nextFreePosition = (void*)pageStart;
}

#endif
