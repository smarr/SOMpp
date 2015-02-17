//
//  Page.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#include <vmobjects/AbstractObject.h>
#include <vm/Universe.h>
#include <interpreter/Interpreter.h>
#include <atomic>
#include <misc/debug.h>
#include <misc/defs.h>
#include <memory/PauselessHeap.h>


void* PauselessPage::operator new(size_t count) {
    assert(count == sizeof(PauselessPage));
    size_t size = GetHeap<PauselessHeap>()->pagedHeap.pageSize;
    assert(size > count);
    
    void* result;
    int r = posix_memalign(&result, size, size);
    assert(r == 0);
    
    return result;
}

PauselessPage::PauselessPage(PauselessHeap* heap)
    : heap(heap), nextFreePosition(buffer),
      bufferEnd((void*)((uintptr_t)buffer + heap->pagedHeap.pageSize - sizeof(PauselessPage))),
      treshold((void*)((size_t)buffer + ((size_t)(heap->pagedHeap.pageSize * 0.9)))),
      canAllocateAndIsNotReturned(true) { }

AbstractVMObject* PauselessPage::allocate(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);

    if (nextFreePosition > bufferEnd) {
        GetUniverse()->ErrorExit("Failed to allocate " + to_string(size) + " Bytes in page.");
    }
    return newObject;
}

AbstractVMObject* PauselessPage::allocateNonRelocatable(size_t size) {
    assert(nonRelocatablePage != nullptr);
    AbstractVMObject* newObject = nonRelocatablePage->allocate(size);
    if (nonRelocatablePage->isFull()) {
        nonRelocatablePage->getNextPage(true);
    }
    return newObject;
}

PauselessPage* PauselessPage::getNextPage(bool nonRelocatable) {
    BaseThread* const thread = GetUniverse()->GetBaseThread();
    PauselessPage* current = thread->GetPage();
    
    if (nonRelocatable) {
        if (current->nonRelocatablePage && current->nonRelocatablePage != this) {
            return current->nonRelocatablePage;
        }
    } else {
        if (current && current != this) {
            return current;
        }
    }
    
    PauselessPage* newPage = heap->pagedHeap.GetNextPage();
    newPage->canAllocateAndIsNotReturned = true;
    
    if (nonRelocatable) {
        assert(current->nonRelocatablePage == this);
        current->nonRelocatablePage = newPage;
    } else {
        assert(current == this);
        newPage->nonRelocatablePage = current->nonRelocatablePage;
        thread->SetPage(reinterpret_cast<Page*>(newPage));
    }
    
    canAllocateAndIsNotReturned = false;
    if (nonRelocatable) {
        heap->AddFullNonRelocatablePage(this);
    } else {
        heap->pagedHeap.ReturnFullPage(this);
    }
    return newPage;
}

AbstractVMObject* PauselessPage::AllocateObject(size_t size, bool nonRelocatable) {
    if (nonRelocatable) {
        return allocateNonRelocatable(size);
    }
    
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);

    if (nextFreePosition > bufferEnd) {
        nextFreePosition = (void*)((size_t)nextFreePosition - size); // reset pointer
        
        PauselessPage* newPage = getNextPage(false);
        newObject = newPage->AllocateObject(size, nonRelocatable);
    }
     
    return newObject;
}

void PauselessPage::ClearPage() {
    nextFreePosition = buffer;
}

#if GC_TYPE==PAUSELESS
void PauselessPage::Block() {
    blocked = true;
    sideArray = new std::atomic<AbstractVMObject*>[((uintptr_t) bufferEnd - (uintptr_t)buffer) / 8];
    for (int i=0; i < (((uintptr_t) bufferEnd - (uintptr_t)buffer) / 8); i++) {
        sideArray[i] = nullptr;
    }
}

void PauselessPage::UnBlock() {
    assert(blocked == true);
    memset(buffer, 0xa, (uintptr_t)bufferEnd - (uintptr_t)buffer);
    blocked = false;
    delete [] sideArray;
}

AbstractVMObject* PauselessPage::LookupNewAddress(AbstractVMObject* oldAddress, BaseThread* thread) {
    uintptr_t position = ((uintptr_t)oldAddress - (uintptr_t)buffer)/8;
    if (!sideArray[position]) {
        AbstractVMObject* newLocation = oldAddress->Clone(this);
        AbstractVMObject* test = nullptr;
        void* oldPosition = thread->GetPage()->nextFreePosition;
        if (!sideArray[position].compare_exchange_strong(test, newLocation)) {
            thread->GetPage()->nextFreePosition = oldPosition;
        }
    }
    assert(Universe::IsValidObject((AbstractVMObject*) sideArray[position]));
    return sideArray[position];
}

void PauselessPage::AddAmountLiveData(size_t objectSize) {
    amountLiveData += objectSize;
}

double PauselessPage::GetPercentageLiveData() {
    return amountLiveData / ((uintptr_t) bufferEnd - (uintptr_t) buffer);
}

void PauselessPage::ResetAmountOfLiveData() {
    amountLiveData = 0;
}

void PauselessPage::RelocatePage() {
    BaseThread* thread = GetUniverse()->GetBaseThread();
    AbstractVMObject* currentObject = reinterpret_cast<AbstractVMObject*>(buffer);
    while (currentObject < nextFreePosition) {
        assert(Universe::IsValidObject(currentObject));
        if (currentObject->GetGCField() == PauselessCollectorThread::GetMarkValue()) {
            AbstractVMObject* newLocation = currentObject->Clone(this);
            uintptr_t positionSideArray = ((uintptr_t)currentObject - (uintptr_t) buffer)/8;
            AbstractVMObject* test = nullptr;
            void* oldPosition = thread->GetPage()->nextFreePosition;
            if (!sideArray[positionSideArray].compare_exchange_strong(test, newLocation)) {
                thread->GetPage()->nextFreePosition = oldPosition;
            }
            assert(Universe::IsValidObject((AbstractVMObject*) sideArray[positionSideArray]));
        }
        
        currentObject = currentObject->GetNextObject();
    }
    amountLiveData = 0;

    ClearPage();
}

#endif
