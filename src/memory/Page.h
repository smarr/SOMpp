//
//  Page.h
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#ifndef SOM_Page_h
#define SOM_Page_h

#include <cstdlib>
#include "PagedHeap.h"

using namespace std;

class Page {
    
public:
    
    Page(void* pageStart, PagedHeap* heap);
    
    AbstractVMObject* AllocateObject(size_t size);
    void ClearPage();
    
    //specifically for pauseless
    void ClearMarkBits();
    
private:
    size_t pageStart;
    size_t pageEnd;
    void* collectionLimit;
    void* volatile nextFreePosition;
    PagedHeap* heap;
    //page stuff specifically for pauseless
    int numberLiveObjects;
    bool protection;
};

#endif
