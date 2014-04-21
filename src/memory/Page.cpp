//
//  Page.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#include "Page.h"
#include "GenerationalHeap.h"
#include "../vm/Universe.h"

Page::Page(void* pageStart, GenerationalHeap* heap) {
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
    if (nextFreePosition > collectionLimit) {
        heap->RelinquishFullPage(this);
        _UNIVERSE->GetInterpreter()->SetPage(_HEAP->RequestPage());
    }
    return newObject;
}

void Page::ClearPage() {
    nextFreePosition = (void*) pageStart;
}
