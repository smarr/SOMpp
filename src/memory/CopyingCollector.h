#pragma once

#include "GarbageCollector.h"
#if GC_TYPE==COPYING

class CopyingCollector: public GarbageCollector {
public:
    CopyingCollector(Heap* h) :
            GarbageCollector(h) {
    }
    ;
private:
    void Collect();
};

#endif
