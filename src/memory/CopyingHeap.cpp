#include <cassert>
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
    collectionLimit = (void*)((size_t)currentBuffer + ((size_t)((double)bufSize * 0.9)));
    nextFreePosition = currentBuffer;

    oldBufferEnd = (void*)((size_t)oldBuffer + bufSize);
    oldBufferIsValid = false;
}

void CopyingHeap::switchBuffers(bool increaseMemory) {
    size_t oldBufSizeBeforeSwitch = (size_t)oldBufferEnd - (size_t)oldBuffer;

    void* oldBufferBeforeSwitch = oldBuffer;
    void* oldBufferEndBeforeSwitch = oldBufferEnd;

    // make current buffer the old one
    oldBuffer = currentBuffer;
    oldBufferEnd = currentBufferEnd;
    oldBufferIsValid = true;

    size_t currentBufSize = 0;
    if (increaseMemory) {
        // increase memory if scheduled in collection before
        free(oldBufferBeforeSwitch);
        oldBufferEndBeforeSwitch = nullptr;

        size_t newSize = oldBufSizeBeforeSwitch * 2;
        currentBuffer = malloc(newSize);

        if (currentBuffer == nullptr) {
            Universe::ErrorExit("unable to allocate heap memory");
        }

        currentBufferEnd = (void*)((size_t)currentBuffer + newSize);
        collectionLimit = (void*)((size_t)currentBuffer + ((size_t)((double)newSize * 0.9)));
        nextFreePosition = currentBuffer;
        currentBufSize = newSize;
    } else {
        currentBuffer = oldBufferBeforeSwitch;
        currentBufferEnd = oldBufferEndBeforeSwitch;
        currentBufSize = oldBufSizeBeforeSwitch;

        collectionLimit = (void*)((size_t)oldBufferBeforeSwitch + (size_t)((double)oldBufSizeBeforeSwitch * 0.9));
        nextFreePosition = currentBuffer;
    }

    // init currentBuffer with zeros
    memset(currentBuffer, 0x0, currentBufSize);
}

void CopyingHeap::invalidateOldBuffer() {
    oldBufferIsValid = false;

    size_t currentBufSize = (size_t)currentBufferEnd - (size_t)currentBuffer;
    size_t oldBufSize = (size_t)oldBufferEnd - (size_t)oldBuffer;

    if (DEBUG) {
        memset(oldBuffer, 0xFF, oldBufSize);
    }

    if (currentBufSize > oldBufSize) {
        free(oldBuffer);

        oldBuffer = malloc(currentBufSize);

        if (oldBuffer == nullptr) {
            Universe::ErrorExit("unable to allocate heap memory");
        }

        oldBufferEnd = (void*)((size_t)oldBuffer + currentBufSize);
    }
}

AbstractVMObject* CopyingHeap::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*) nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if (nextFreePosition > currentBufferEnd) {
        ErrorPrint("Failed to allocate " + to_string(size) + " Bytes.\n");
        Universe::Quit(-1);
    }

    // let's see if we have to trigger the GC
    if (nextFreePosition > collectionLimit) {
        requestGC();
    }

    return newObject;
}

bool CopyingHeap::IsInCurrentBuffer(AbstractVMObject* obj) {
    if (obj == nullptr) {
        return true;
    }

    size_t objAddress = (size_t) obj;
    return (size_t) currentBuffer <= objAddress && objAddress < (size_t) currentBufferEnd;
}

bool CopyingHeap::IsInOldBufferAndOldBufferIsValid(AbstractVMObject* obj) {
    if (obj == nullptr) {
        return true;
    }

    if (!oldBufferIsValid) {
        assert(oldBufferIsValid);
        return false;
    }

    size_t objAddress = (size_t) obj;
    assert((size_t) oldBuffer <= objAddress);
    assert(objAddress < (size_t) oldBufferEnd);

    return (size_t) oldBuffer <= objAddress && objAddress < (size_t) oldBufferEnd;
}
