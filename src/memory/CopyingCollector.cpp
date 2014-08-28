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
    size_t newSize = ((size_t)(GetHeap()->currentBufferEnd) -
            (size_t)(GetHeap()->currentBuffer)) * 2;

    GetHeap()->switchBuffers();
    //increase memory if scheduled in collection before
    if (increaseMemory)
    {
        free(GetHeap()->currentBuffer);
        GetHeap()->currentBuffer = malloc(newSize);
        GetHeap()->nextFreePosition = GetHeap()->currentBuffer;
        GetHeap()->collectionLimit = (void*)((size_t)(GetHeap()->currentBuffer) +
                (size_t)(0.9 * newSize));
        GetHeap()->currentBufferEnd = (void*)((size_t)(GetHeap()->currentBuffer) +
                newSize);
        if (GetHeap()->currentBuffer == NULL)
        GetUniverse()->ErrorExit("unable to allocate more memory");
    }
    //init currentBuffer with zeros
    memset(GetHeap()->currentBuffer, 0x0, (size_t)(GetHeap()->currentBufferEnd) -
            (size_t)(GetHeap()->currentBuffer));
    GetUniverse()->WalkGlobals(copy_if_necessary);
    pVMFrame currentFrame = GetUniverse()->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        pVMFrame newFrame = static_cast<pVMFrame>(copy_if_necessary(currentFrame));
        GetUniverse()->GetInterpreter()->SetFrame(newFrame);
    }

    //now copy all objects that are referenced by the objects we have moved so far
    VMOBJECT_PTR curObject = (VMOBJECT_PTR)(GetHeap()->currentBuffer);
    while (curObject < GetHeap()->nextFreePosition) {
        curObject->WalkObjects(copy_if_necessary);
        curObject = (VMOBJECT_PTR)((size_t)curObject + curObject->GetObjectSize());
    }
    //increase memory if scheduled in collection before
    if (increaseMemory)
    {
        increaseMemory = false;
        free(GetHeap()->oldBuffer);
        GetHeap()->oldBuffer = malloc(newSize);
        if (GetHeap()->oldBuffer == NULL)
        GetUniverse()->ErrorExit("unable to allocate more memory");
    }

    //if semispace is still 50% full after collection, we have to realloc
    //  bigger ones -> done in next collection
    if ((size_t)(GetHeap()->nextFreePosition) - (size_t)(GetHeap()->currentBuffer) >=
            (size_t)(GetHeap()->currentBufferEnd) -
            (size_t)(GetHeap()->nextFreePosition)) {
        increaseMemory = true;
    }

    Timer::GCTimer->Halt();
}

#endif
