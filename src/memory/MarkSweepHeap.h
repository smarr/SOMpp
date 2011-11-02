
#pragma once

#include "../misc/defs.h"
#if GC_TYPE == MARK_SWEEP

#include "Heap.h"

class MarkSweepHeap : public Heap {
  friend class MarkSweepCollector;
 public:
  MarkSweepHeap(int objectSpaceSize = 1048576);
  AbstractVMObject* AllocateObject(size_t size);
 private:
    vector<pVMObject>* allocatedObjects;
    uint32_t spcAlloc;
    uint32_t collectionLimit;

};

#endif
