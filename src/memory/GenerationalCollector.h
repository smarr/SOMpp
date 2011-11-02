#pragma once
#include "../misc/defs.h"
#if GC_TYPE == GENERATIONAL

#include "GarbageCollector.h"

class GenerationalCollector : public GarbageCollector {
 public:
  GenerationalCollector(Heap* heap);
  void Collect();
 private:
  int32_t majorCollectionThreshold;
  int32_t matureObjectsSize;
  void MajorCollection();
  void MinorCollection();  
};

#endif
