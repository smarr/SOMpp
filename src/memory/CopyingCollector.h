#pragma once

#include "GarbageCollector.h"

class CopyingHeap;

class CopyingCollector: public GarbageCollector<CopyingHeap> {
public:
    CopyingCollector(CopyingHeap* h) : GarbageCollector(h) {};
    virtual ~CopyingCollector() {}
    
    virtual void Collect();

};
