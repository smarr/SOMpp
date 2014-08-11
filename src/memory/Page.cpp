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

Page::Page(void* pageStart, PagedHeap* heap) {
#if GC_TYPE==PAUSELESS
    //used = false;
#endif
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
        cout << "Failed to allocate " << size << " Bytes in page." << endl;
        _UNIVERSE->Quit(-1);
    }
//#if GC_TYPE==PAUSELESS -> as soon as a page is requested, the page will be marked as not suitable for cleaning up
//    used = true;
//#endif
    if (nextFreePosition > collectionLimit) {
        heap->RelinquishFullPage(this);
        _UNIVERSE->GetInterpreter()->SetPage(heap->RequestPage());
    }
    return newObject;
}

void Page::ClearPage() {
    nextFreePosition = (void*) pageStart;
}

#if GC_TYPE==PAUSELESS
void Page::Block() {
    blocked = true;
    sideArray = new pVMObject[PAGE_SIZE / 8];
    for (int i=0; i < (PAGE_SIZE / 8); i++) {
        sideArray[i] = NULL;
    }
}

void Page::UnBlock() {
    blocked = false;
    delete [] sideArray;
}

bool Page::Blocked() {
    return blocked;
}

pVMObject Page::LookupNewAddress(VMOBJECT_PTR oldAddress) {
    long position = ((size_t)oldAddress - pageStart)/8;
    if (!sideArray[position]) {
        oldAddress->ProtectedClone(_PAGE);
    }
    return sideArray[position];
}


void Page::AddAmountLiveData(size_t objectSize) {
    amountLiveData += objectSize;
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
*/