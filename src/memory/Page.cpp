//
//  Page.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#include "Page.h"
#include "../vm/Universe.h"


/*
Page::Page(void* pageStart, PagedHeap* heap) {
    this->nextFreePosition = pageStart;
    this->pageStart = (size_t)pageStart;
    //this->pageEnd = this->pageStart + heap->pageSize;
}

AbstractVMObject* Page::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if ((size_t)nextFreePosition > pageEnd) {
        cout << "Failed to allocate " << size << " Bytes in page." << endl;
        _UNIVERSE->Quit(-1);
    } */
    /*
    if (nextFreePosition > collectionLimit)
        _UNIVERSE->GetInterpreter()->SetPage(_HEAP->RequestPage()); //RequestPage gaat sowieso lukken
    */
  /*  return newObject;
}
*/