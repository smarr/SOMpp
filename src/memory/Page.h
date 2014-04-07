//
//  Page.h
//  SOM
//
//  Created by Jeroen De Geeter on 6/04/14.
//
//

#ifndef SOM_Page_h
#define SOM_Page_h

#include "../vmobjects/VMObjectBase.h"

class Page {
    
public:
    Page(void*);
    
    AbstractVMObject* AllocateObject(size_t size);
    
private:
    static int pageSize;
    
    size_t pageStart;
    size_t pageEnd;
    void* volatile nextFreePosition;
    int numberLiveObjects;
    int pageNumber;
};


#endif
