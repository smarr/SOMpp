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


PauselessPage::PauselessPage(void* pageStart, PagedHeap* heap) {
    this->heap = heap;
    this->nextFreePosition = pageStart;
    this->pageStart = (size_t)pageStart;
    this->pageEnd = this->pageStart + PAGE_SIZE;
    treshold = (void*)((size_t)pageStart + ((size_t)(PAGE_SIZE * 0.9)));
}

AbstractVMObject* PauselessPage::allocate(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);

    if ((size_t)nextFreePosition > pageEnd) {
        sync_out(ostringstream() << "Failed to allocate " << size << " Bytes in page.");
        GetUniverse()->Quit(-1);
    }
    return newObject;
}

AbstractVMObject* PauselessPage::allocateNonRelocatable(size_t size) {
    assert(nonRelocatablePage != nullptr);
    AbstractVMObject* newObject = nonRelocatablePage->allocate(size);
    if (nonRelocatablePage->isFull()) {
        assert(dynamic_cast<PauselessHeap*>(heap));
        static_cast<PauselessHeap*>(heap)->AddFullNonRelocatablePage(nonRelocatablePage);
        nonRelocatablePage = heap->RequestPage();
    }
    return newObject;
}

AbstractVMObject* PauselessPage::AllocateObject(size_t size ALLOC_OUTSIDE_NURSERY_DECLpp ALLOC_NON_RELOCATABLE_DECLpp) {
    if (nonRelocatable) {
        return allocateNonRelocatable(size);
    }
    
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);

    if ((size_t)nextFreePosition > pageEnd) {
        nextFreePosition = (void*)((size_t)nextFreePosition - size); // reset pointer
        
        heap->RelinquishPage(this);
        PauselessPage* newPage = heap->RequestPage();
        GetUniverse()->GetInterpreter()->SetPage(reinterpret_cast<Page*>(newPage));
        newPage->SetNonRelocatablePage(nonRelocatablePage);
    }
     
    return newObject;
}

void PauselessPage::ClearPage() {
    nextFreePosition = (void*) pageStart;
}

#if GC_TYPE==PAUSELESS
void PauselessPage::Block() {
    blocked = true;
    sideArray = new std::atomic<AbstractVMObject*>[PAGE_SIZE / 8];
    for (int i=0; i < (PAGE_SIZE / 8); i++) {
        sideArray[i] = nullptr;
    }
}

void PauselessPage::UnBlock() {
    assert(blocked == true);
    memset((void*)pageStart, 0xa, PAGE_SIZE);
    blocked = false;
    delete [] sideArray;
}

AbstractVMObject* PauselessPage::LookupNewAddress(AbstractVMObject* oldAddress, Interpreter* thread) {
    long position = ((size_t)oldAddress - pageStart)/8;
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

AbstractVMObject* PauselessPage::LookupNewAddress(AbstractVMObject* oldAddress, PauselessCollectorThread* thread) {
    long position = ((size_t)oldAddress - pageStart)/8;
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
    //pthread_mutex_lock
    amountLiveData += objectSize;
    //pthread_mutex_unlock
}

double PauselessPage::GetPercentageLiveData() {
    return amountLiveData / PAGE_SIZE;
}

void PauselessPage::ResetAmountOfLiveData() {
    amountLiveData = 0;
}

void PauselessPage::Free(size_t numBytes) {
    nextFreePosition = (void*)((size_t)nextFreePosition - numBytes);
}

void PauselessPage::RelocatePage() {
    PauselessCollectorThread* thread = _HEAP->GetGCThread();
    for (AbstractVMObject* currentObject = (AbstractVMObject*) pageStart;
         (size_t) currentObject < (size_t) nextFreePosition;
         currentObject = (AbstractVMObject*) (currentObject->GetObjectSize() + (size_t) currentObject)) {
        assert(Universe::IsValidObject(currentObject));
        if (currentObject->GetGCField() == _HEAP->GetMarkValue()) {
            AbstractVMObject* newLocation = currentObject->Clone(this);
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
