#include "PauselessHeap.h"
#include "Page.h"
#include "../vmobjects/AbstractObject.h"
#include "../vm/Universe.h"

#include <string.h>
#include <iostream>

#include <sys/mman.h>

#if GC_TYPE == PAUSELESS

PauselessHeap::PauselessHeap(long objectSpaceSize, long pageSize) {
    //gc = new PauselessCollector(this);
}

#endif
