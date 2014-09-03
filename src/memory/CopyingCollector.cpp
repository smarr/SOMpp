#include "../misc/defs.h"

#include "CopyingHeap.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/VMFrame.h"

#include "CopyingCollector.h"

static oop_t copy_if_necessary(oop_t obj) {
    // don't process tagged objects
    if (IS_TAGGED(obj))
        return obj;

    long gcField = obj->GetGCField();
    //GCField is abused as forwarding pointer here
    //if someone has moved before, return the moved object
    if (gcField != 0)
        return (oop_t) gcField;
    
    //we have to clone ourselves
    oop_t newObj = obj->Clone();
    
#ifndef NDEBUG
    obj->MarkObjectAsInvalid();
#endif
    
    obj->SetGCField((long)newObj);
    return newObj;
}

void CopyingCollector::Collect() {
    Timer::GCTimer->Resume();
    //reset collection trigger
    heap->resetGCTrigger();

    static bool increaseMemory;
    size_t newSize = ((size_t)(heap->currentBufferEnd) -
            (size_t)(heap->currentBuffer)) * 2;

    heap->switchBuffers();

    // increase memory if scheduled in collection before
    if (increaseMemory) {
        free(heap->currentBuffer);
        heap->currentBuffer = malloc(newSize);
        heap->nextFreePosition = heap->currentBuffer;
        heap->collectionLimit = (void*)((size_t)(heap->currentBuffer) +
                (size_t)(0.9 * newSize));
        heap->currentBufferEnd = (void*)((size_t)(heap->currentBuffer) +
                newSize);
        if (heap->currentBuffer == NULL)
            GetUniverse()->ErrorExit("unable to allocate more memory");
    }
    
    // init currentBuffer with zeros
    memset(heap->currentBuffer, 0x0, (size_t)(heap->currentBufferEnd) -
            (size_t)(heap->currentBuffer));
    GetUniverse()->WalkGlobals(copy_if_necessary);
    pVMFrame currentFrame = GetUniverse()->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        pVMFrame newFrame = static_cast<pVMFrame>(copy_if_necessary(currentFrame));
        GetUniverse()->GetInterpreter()->SetFrame(newFrame);
    }

    //now copy all objects that are referenced by the objects we have moved so far
    oop_t curObject = (oop_t)(heap->currentBuffer);
    while (curObject < heap->nextFreePosition) {
        curObject->WalkObjects(copy_if_necessary);
        curObject = (oop_t)((size_t)curObject + curObject->GetObjectSize());
    }
    
    //increase memory if scheduled in collection before
    if (increaseMemory) {
        increaseMemory = false;
        free(heap->oldBuffer);
        heap->oldBuffer = malloc(newSize);
        if (heap->oldBuffer == NULL)
            GetUniverse()->ErrorExit("unable to allocate more memory");
    }

    // if semispace is still 50% full after collection, we have to realloc
    //  bigger ones -> done in next collection
    if ((size_t)(heap->nextFreePosition) - (size_t)(heap->currentBuffer) >=
            (size_t)(heap->currentBufferEnd) -
            (size_t)(heap->nextFreePosition)) {
        increaseMemory = true;
    }

    Timer::GCTimer->Halt();
}
