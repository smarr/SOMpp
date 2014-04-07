//
//  Page.cpp
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#include "Page.h"
#include "../vm/Universe.h"

//using namespace std;

Page::Page(void* pageStart) {
    this->pageStart = pageStart;
    this->pageEnd = this->pageStart + pageSize;
    this->nextFreePosition = pageStart;
}


AbstractVMObject* Page::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if ((size_t)nextFreePosition > pageEnd) {
        cout << "Failed to allocate " << size << " Bytes in page." << endl;
        _UNIVERSE->Quit(-1);
    }
    //
    //if (nextFreePosition > collectionLimit)
        //aangeven dat hij van page moet switchen
    return newObject;
}