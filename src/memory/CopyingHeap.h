#pragma once

#include "Heap.h"
#include <string.h>
#include <mutex>


using namespace std;

class CopyingHeap : public Heap<CopyingHeap> {
    friend class CopyingCollector;
public:
    CopyingHeap(size_t pageSize, size_t maxHeapSize);
    AbstractVMObject* AllocateObject(size_t size);
private:
    void switchBuffers(void);
    
    void* currentBuffer;
    void* collectionLimit;
    void* oldBuffer;
    void* currentBufferEnd;
    void* nextFreePosition;
    
    mutex allocation_mutex;
};
