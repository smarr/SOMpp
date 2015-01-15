#include "MarkSweepCollector.h"

#if GC_TYPE==MARK_SWEEP

#include "../vm/Universe.h"
#include "MarkSweepHeap.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/VMFrame.h"

#define GC_MARKED 3456

void MarkSweepCollector::Collect() {
    MarkSweepHeap* heap = _HEAP;
    Timer::GCTimer->Resume();

    //now mark all reachables
    markReachableObjects();

    //in this survivors stack we will remember all objects that survived
    vector<VMOBJECT_PTR>* survivors = new vector<VMOBJECT_PTR>();
    size_t survivorsSize = 0;

    vector<VMOBJECT_PTR>::iterator iter;
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
    
    //reset collection trigger
    heap->resetGCTrigger();
    
    Timer::GCTimer->Halt();
}

VMOBJECT_PTR mark_object(VMOBJECT_PTR obj) {
#ifdef USE_TAGGING
    if (IS_TAGGED(obj))
    return obj;
#endif
    if (obj->GetGCField())
    return obj;

    obj->SetGCField(GC_MARKED);
    obj->WalkObjects(mark_object);
    return obj;
}

void MarkSweepCollector::markReachableObjects() {
    _UNIVERSE->WalkGlobals(mark_object);
    MarkInterpretersFrameAndThread()
}

void MarkSweepCollector::MarkInterpretersFrameAndThread() {
    vector<Interpreter*>* interpreters = _UNIVERSE->GetInterpreters();
    for (std::vector<Interpreter*>::iterator it = interpreters->begin() ; it != interpreters->end(); ++it) {
        // Get the current frame and thread of each interpreter and mark it.
        // Since marking is done recursively, this automatically
        // marks the whole stack
        VMFrame* currentFrame = (*it)->GetFrame();
        if (currentFrame != NULL) {
            VMFrame* newFrame = static_cast<VMFrame*>(mark_object(currentFrame));
            (*it)->SetFrame(newFrame);
            
        }
        VMThread* currentThread = (*it)->GetThread();
        if (currentThread != NULL) {
            VMThread* newThread = static_cast<VMThread*>(mark_object(currentThread));
            (*it)->SetThread(newThread);
        }
    }
}

#endif
