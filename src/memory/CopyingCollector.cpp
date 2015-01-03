#include "../misc/defs.h"

#include "CopyingHeap.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/VMFrame.h"
#include <vmobjects/IntegerBox.h>

#include "CopyingCollector.h"

static gc_oop_t copy_if_necessary(gc_oop_t oop) {
    // don't process tagged objects
    if (IS_TAGGED(oop))
        return oop;
    
    AbstractVMObject* obj = AS_OBJ(oop);
    assert(Universe::IsValidObject(obj));


    long gcField = obj->GetGCField();
    //GCField is abused as forwarding pointer here
    //if someone has moved before, return the moved object
    if (gcField != 0)
        return (gc_oop_t) gcField;
    
    // we have to clone ourselves
    AbstractVMObject* newObj = obj->Clone();
    
    if (DEBUG)
        obj->MarkObjectAsInvalid();
    
    obj->SetGCField((long)newObj);
#warning not sure about the use of _store_ptr here, or whether it should be a plain cast
    return _store_ptr(newObj);
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
        if (heap->currentBuffer == nullptr)
            GetUniverse()->ErrorExit("unable to allocate more memory");
    }
    
    // init currentBuffer with zeros
    memset(heap->currentBuffer, 0x0, (size_t)(heap->currentBufferEnd) -
            (size_t)(heap->currentBuffer));
    GetUniverse()->WalkGlobals(copy_if_necessary);

    //now copy all objects that are referenced by the objects we have moved so far
    AbstractVMObject* curObject = (AbstractVMObject*)(heap->currentBuffer);
    while (curObject < heap->nextFreePosition) {
        curObject->WalkObjects(copy_if_necessary);
        curObject = (AbstractVMObject*)((size_t)curObject + curObject->GetObjectSize());
    }
    
    //increase memory if scheduled in collection before
    if (increaseMemory) {
        increaseMemory = false;
        free(heap->oldBuffer);
        heap->oldBuffer = malloc(newSize);
        if (heap->oldBuffer == nullptr)
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
