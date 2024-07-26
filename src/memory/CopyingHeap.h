#pragma once

#include <cstring>

#include "Heap.h"

class CopyingHeap : public Heap<CopyingHeap> {
    friend class CopyingCollector;
public:
    CopyingHeap(size_t objectSpaceSize);
    AbstractVMObject* AllocateObject(size_t size);
private:
    void* currentBuffer;
    void* collectionLimit;
    void* oldBuffer;
    void* currentBufferEnd;
    void switchBuffers(void);
    void* nextFreePosition;
};
