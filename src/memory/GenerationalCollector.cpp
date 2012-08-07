#include "../misc/defs.h"
#if GC_TYPE==GENERATIONAL

#include "GenerationalCollector.h"

#include "Heap.h"
#include "../vm/Universe.h"
#ifdef USE_TAGGING
#include "../vmobjects/VMIntPointer.h"
#endif
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMEvaluationPrimitive.h"



#define INITIAL_MAJOR_COLLECTION_THRESHOLD (5 * 1024 * 1024) //5 MB

GenerationalCollector::GenerationalCollector(Heap* heap) : GarbageCollector(heap) {
	majorCollectionThreshold = INITIAL_MAJOR_COLLECTION_THRESHOLD; 
	matureObjectsSize = 0;
}


VMOBJECT_PTR mark_object(VMOBJECT_PTR obj) {
#ifdef USE_TAGGING
	//don't process tagged objects
	if ((size_t)((void*)obj) & 1)
		return obj;
#endif
    if (obj->GetGCField() & MASK_OBJECT_IS_MARKED)
        return (obj);
    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object);
    return obj;
}

VMOBJECT_PTR copy_if_necessary(VMOBJECT_PTR obj) {
#ifdef USE_TAGGING
  //don't process tagged objects
  if ((size_t)((void*)obj) & 0x1)
    return obj;
#endif
  size_t gcField = obj->GetGCField();
  //if this is an old object already, we don't have to copy
  if (gcField & MASK_OBJECT_IS_OLD)
    return obj;
  //GCField is abused as forwarding pointer here
  //if someone has moved before, return the moved object
  if (gcField != 0)
    return (VMOBJECT_PTR)gcField;
  //we have to clone ourselves
  VMOBJECT_PTR newObj = obj->Clone();
    obj->SetGCField((size_t)newObj);
  newObj->SetGCField(MASK_OBJECT_IS_OLD);
  //walk recursively
  newObj->WalkObjects(copy_if_necessary);
  return newObj;
}

void GenerationalCollector::MinorCollection() {
  //walk all globals
  _UNIVERSE->WalkGlobals(&copy_if_necessary);
  //and the current frame
  pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
  if (currentFrame != NULL) {
    pVMFrame newFrame = static_cast<pVMFrame>(copy_if_necessary(currentFrame));
    _UNIVERSE->GetInterpreter()->SetFrame(newFrame);
  }

  //and also all objects that have been detected by the write barriers
  for (vector<size_t>::iterator objIter =
       _HEAP->oldObjsWithRefToYoungObjs->begin(); objIter !=
       _HEAP->oldObjsWithRefToYoungObjs->end(); objIter++) {
    //content of oldObjsWithRefToYoungObjs is not altered while iteration,
    // because copy_if_necessary returns old objs only -> ignored by
    // write_barrier
    VMOBJECT_PTR obj = (VMOBJECT_PTR)(*objIter);
    obj->SetGCField(MASK_OBJECT_IS_OLD);
    obj->WalkObjects(&copy_if_necessary);
  }
  _HEAP->oldObjsWithRefToYoungObjs->clear();
  _HEAP->nextFreePosition = _HEAP->nursery;
}

void GenerationalCollector::MajorCollection() {
    //first we have to mark all objects (globals and current frame recursively)
    _UNIVERSE->WalkGlobals(&mark_object);
    //and the current frame
    pVMFrame currentFrame = _UNIVERSE->GetInterpreter()->GetFrame();
    if (currentFrame != NULL) {
        pVMFrame newFrame = static_cast<pVMFrame>(mark_object(currentFrame));
        _UNIVERSE->GetInterpreter()->SetFrame(newFrame);
    }

    //now that all objects are marked we can safely delete all allocated objects that are not marked
    vector<VMOBJECT_PTR>* survivors = new vector<VMOBJECT_PTR>();
	for (vector<VMOBJECT_PTR>::iterator objIter =
			_HEAP->allocatedObjects->begin(); objIter !=
			_HEAP->allocatedObjects->end(); objIter++) {
		if ((*objIter)->GetGCField() & MASK_OBJECT_IS_MARKED) {
			survivors->push_back(*objIter);
			(*objIter)->SetGCField(MASK_OBJECT_IS_OLD);
		}
		else {
			_HEAP->FreeObject(*objIter);
		}
    }
	delete _HEAP->allocatedObjects;
	_HEAP->allocatedObjects = survivors;
}


void GenerationalCollector::Collect() {
  Timer::GCTimer->Resume();
  //reset collection trigger
  heap->resetGCTrigger();

  MinorCollection();
  if (_HEAP->matureObjectsSize > majorCollectionThreshold)
  {
    MajorCollection();
    majorCollectionThreshold = 2 * _HEAP->matureObjectsSize;

  }
  Timer::GCTimer->Halt();
}

#endif
