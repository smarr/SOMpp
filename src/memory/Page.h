//
//  Page.h
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#ifndef SOM_Page_h
#define SOM_Page_h

#include "PagedHeap.h"

class GenerationalHeap;

using namespace std;

class Page {
    
public:
    
    Page(void* pageStart, GenerationalHeap* heap);
    
    AbstractVMObject* AllocateObject(size_t size);
    void ClearPage();
    
private:
    size_t pageStart;
    size_t pageEnd;
    void* collectionLimit;
    void* volatile nextFreePosition;
    int numberLiveObjects;
    int pageNumber;
    GenerationalHeap* heap;
};

#endif
