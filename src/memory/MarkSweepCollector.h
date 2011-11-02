#pragma once

#include "../misc/defs.h"
#if GC_TYPE == MARK_SWEEP

#include "GarbageCollector.h"

class MarkSweepCollector : public GarbageCollector {
 public:
  MarkSweepCollector(Heap* heap) : GarbageCollector(heap) {
  }
	void Collect();
 private:
	void markReachableObjects();
};

#endif
