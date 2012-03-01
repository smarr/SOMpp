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
	//reset collection trigger
	heap->resetGCTrigger();

	//now mark all reachables
	markReachableObjects();

	//in this survivors stack we will remember all objects that survived
	vector<pVMObject>* survivors = new vector<pVMObject>();
	size_t survivorsSize = 0;

	vector<pVMObject>::iterator iter;
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


VMOBJECT_PTR mark_object(VMOBJECT_PTR obj) {
#ifdef USE_TAGGING
	if ((size_t)((void*)obj) & 1)
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
  // Get the current frame and mark it.
  // Since marking is done recursively, this automatically
  // marks the whole stack
  pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
  if (currentFrame != NULL) {
#ifdef USE_TAGGING
    pVMFrame newFrame = static_cast<VMFrame*>(mark_object(currentFrame));
#else
    pVMFrame newFrame = static_cast<pVMFrame>(mark_object(currentFrame));
#endif
    _UNIVERSE->GetInterpreter()->SetFrame(newFrame);
  }
}
#endif
