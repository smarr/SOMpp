#pragma once

#include "Heap.h"
#include <string.h>

class CopyingHeap : public Heap<CopyingHeap> {
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
