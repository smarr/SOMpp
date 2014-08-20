#include "../misc/defs.h"

#if GC_TYPE==COPYING

#include "CopyingHeap.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/VMFrame.h"

#include "CopyingCollector.h"

VMOBJECT_PTR copy_if_necessary(VMOBJECT_PTR obj) {
#ifdef USE_TAGGING
    //don't process tagged objects
    if (IS_TAGGED(obj))
        return obj;
#endif
    long gcField = obj->GetGCField();
    //GCField is abused as forwarding pointer here
    //if someone has moved before, return the moved object
    if (gcField != 0)
        return (VMOBJECT_PTR) gcField;
    //we have to clone ourselves
    VMOBJECT_PTR newObj = obj->Clone();
    
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
    size_t newSize = ((size_t)(_HEAP->currentBufferEnd) -
            (size_t)(_HEAP->currentBuffer)) * 2;

    _HEAP->switchBuffers();
    //increase memory if scheduled in collection before
    if (increaseMemory)
    {
        free(_HEAP->currentBuffer);
        _HEAP->currentBuffer = malloc(newSize);
        _HEAP->nextFreePosition = _HEAP->currentBuffer;
        _HEAP->collectionLimit = (void*)((size_t)(_HEAP->currentBuffer) +
                (size_t)(0.9 * newSize));
        _HEAP->currentBufferEnd = (void*)((size_t)(_HEAP->currentBuffer) +
                newSize);
        if (_HEAP->currentBuffer == NULL)
        GetUniverse()->ErrorExit("unable to allocate more memory");
    }
    //init currentBuffer with zeros
    memset(_HEAP->currentBuffer, 0x0, (size_t)(_HEAP->currentBufferEnd) -
            (size_t)(_HEAP->currentBuffer));
    GetUniverse()->WalkGlobals(copy_if_necessary);
    pVMFrame currentFrame = GetUniverse()->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        pVMFrame newFrame = static_cast<pVMFrame>(copy_if_necessary(currentFrame));
        GetUniverse()->GetInterpreter()->SetFrame(newFrame);
    }

    //now copy all objects that are referenced by the objects we have moved so far
    VMOBJECT_PTR curObject = (VMOBJECT_PTR)(_HEAP->currentBuffer);
    while (curObject < _HEAP->nextFreePosition) {
        curObject->WalkObjects(copy_if_necessary);
        curObject = (VMOBJECT_PTR)((size_t)curObject + curObject->GetObjectSize());
    }
    //increase memory if scheduled in collection before
    if (increaseMemory)
    {
        increaseMemory = false;
        free(_HEAP->oldBuffer);
        _HEAP->oldBuffer = malloc(newSize);
        if (_HEAP->oldBuffer == NULL)
        GetUniverse()->ErrorExit("unable to allocate more memory");
    }

    //if semispace is still 50% full after collection, we have to realloc
    //  bigger ones -> done in next collection
    if ((size_t)(_HEAP->nextFreePosition) - (size_t)(_HEAP->currentBuffer) >=
            (size_t)(_HEAP->currentBufferEnd) -
            (size_t)(_HEAP->nextFreePosition)) {
        increaseMemory = true;
    }

    Timer::GCTimer->Halt();
}

#endif
