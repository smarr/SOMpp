#include "DebugCopyingHeap.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <string>

#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"

void DebugCopyingHeap::switchBuffers(bool increaseMemory) {
    assert(
        oldHeap.empty() &&
        "At this point, I'd assume the old heap is empty, because we want to "
        "use it to store into now");

    oldHeap.swap(currentHeap);
    oldHeapIsValid = true;
    oldHeapSize = currentHeapSize;

    if (increaseMemory) {
        currentHeapSize += currentHeapSize;
        collectionLimit = (double)currentHeapSize * 0.9;
    }

    currentHeapUsage = 0;
    assert(currentHeap.empty() &&
           "at the end, the current heap is expected to be empty");
}

void DebugCopyingHeap::invalidateOldBuffer() {
    oldHeapIsValid = false;

    for (AbstractVMObject*& obj : oldHeap) {
        // I was thinking I can check here that the objects all have been
        // invalidated, but of course I can't
        // because only the reachable objects will have been invalidated
        // but, the objects that were not reachable, well, they just remained
        // in the heap.
        free(obj);
    }

    oldHeap.clear();

    if (currentHeapSize > oldHeapSize) {
        oldHeapSize = currentHeapSize;
    }
}

AbstractVMObject* DebugCopyingHeap::AllocateObject(size_t size) {
    AbstractVMObject* newObject = (AbstractVMObject*)malloc(size);
    currentHeap.push_back(newObject);

    currentHeapUsage += size;

    if (currentHeapUsage > currentHeapSize) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Universe::Quit(-1);
    }

    // let's see if we have to trigger the GC
    if (currentHeapUsage > collectionLimit) {
        requestGC();
    }

    return newObject;
}

bool DebugCopyingHeap::IsInCurrentBuffer(AbstractVMObject* obj) {
    if (obj == nullptr) {
        return true;
    }

    return find(currentHeap.begin(), currentHeap.end(), obj) !=
           currentHeap.end();
}

bool DebugCopyingHeap::IsInOldBufferAndOldBufferIsValid(AbstractVMObject* obj) {
    if (obj == nullptr) {
        return true;
    }

    if (!oldHeapIsValid) {
        assert(oldHeapIsValid);
        return false;
    }

    return find(oldHeap.begin(), oldHeap.end(), obj) != oldHeap.end();
}
