#pragma once

#include "Heap.h"
#include <string.h>

#if GC_TYPE==COPYING

class CopyingHeap : public Heap {
  friend class CopyingCollector;
 public:
  CopyingHeap(long heapSize);
  AbstractVMObject* AllocateObject(size_t size);
 private:
  void* currentBuffer;
  void* collectionLimit;
  void* oldBuffer;
  void* currentBufferEnd;
  void switchBuffers(void);
  void* nextFreePosition;
};
#endif
