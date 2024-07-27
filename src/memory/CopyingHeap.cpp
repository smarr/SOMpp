#include <cstdlib>
#include <cstring>
#include <string>

#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "CopyingCollector.h"
#include "CopyingHeap.h"
#include "Heap.h"

CopyingHeap::CopyingHeap(size_t objectSpaceSize) : Heap<CopyingHeap>(new CopyingCollector(this)) {
    size_t bufSize = objectSpaceSize;
    currentBuffer = malloc(bufSize);
    oldBuffer = malloc(bufSize);
    memset(currentBuffer, 0x0, bufSize);
    memset(oldBuffer, 0x0, bufSize);
    currentBufferEnd = (void*)((size_t)currentBuffer + bufSize);
    collectionLimit = (void*)((size_t)currentBuffer + ((size_t)(bufSize *
                            0.9)));
    nextFreePosition = currentBuffer;
}

void CopyingHeap::switchBuffers() {
    size_t bufSize = (size_t)currentBufferEnd - (size_t)currentBuffer;
    void* tmp = oldBuffer;
    oldBuffer = currentBuffer;
    currentBuffer = tmp;
    currentBufferEnd = (void*)((size_t)currentBuffer + bufSize);
    nextFreePosition = currentBuffer;
    collectionLimit = (void*)((size_t)currentBuffer + (size_t)(0.9 *
                    bufSize));
}

AbstractVMObject* CopyingHeap::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if (nextFreePosition > currentBufferEnd) {
        ErrorPrint("Failed to allocate " + to_string(size) + " Bytes.\n");
        Universe::Quit(-1);
    }
    //let's see if we have to trigger the GC
    if (nextFreePosition > collectionLimit) {
        requestGC();
    }
    return newObject;
}
