#include "MarkSweepCollector.h"

#include "../vm/Universe.h"
#include "MarkSweepHeap.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/VMFrame.h"
#include <vmobjects/IntegerBox.h>

#define GC_MARKED 3456

void MarkSweepCollector::Collect() {
    MarkSweepHeap* heap = GetHeap<MarkSweepHeap>();
    Timer::GCTimer->Resume();
    //reset collection trigger
    heap->resetGCTrigger();

    //now mark all reachables
    markReachableObjects();

    //in this survivors stack we will remember all objects that survived
    auto survivors = new vector<AbstractVMObject*>();
    size_t survivorsSize = 0;

    vector<AbstractVMObject*>::iterator iter;
    for (iter = heap->allocatedObjects->begin(); iter !=
            heap->allocatedObjects->end(); iter++) {
        if ((*iter)->GetGCField() == GC_MARKED) {
            //object ist marked -> let it survive
            survivors->push_back(*iter);
            survivorsSize += (*iter)->GetObjectSize();
            (*iter)->SetGCField(0);
        } else {
            //not marked -> kill it
            heap->FreeObject(*iter);
        }
    }

    delete heap->allocatedObjects;
    heap->allocatedObjects = survivors;

    heap->spcAlloc = survivorsSize;
    //TODO: Maybe choose another constant to calculate new collectionLimit here
    heap->collectionLimit = 2 * survivorsSize;
    Timer::GCTimer->Halt();
}

static gc_oop_t mark_object(gc_oop_t oop) {
    if (IS_TAGGED(oop))
        return oop;
    
    AbstractVMObject* obj = AS_OBJ(oop);

    if (obj->GetGCField())
        return oop;

    obj->SetGCField(GC_MARKED);
    obj->WalkObjects(mark_object);
    return oop;
}

void MarkSweepCollector::markReachableObjects() {
    // This walks the globals of the universe, and the interpreter
    GetUniverse()->WalkGlobals(mark_object);
}
