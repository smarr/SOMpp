//
//  Page.h
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#ifndef SOM_Page_h
#define SOM_Page_h

#include "../vmobjects/AbstractObject.h"
#include "PagedHeap.h"

using namespace std;

class Page {
    
public:
    //Page(void* pageStart, PagedHeap* heap);
    
    //AbstractVMObject* AllocateObject(size_t size);
    //static void SetPageSize(long pageSize);
    
private:
    size_t pageStart;
    size_t pageEnd;
    void* volatile nextFreePosition;
    int numberLiveObjects;
    int pageNumber;
};


#endif
